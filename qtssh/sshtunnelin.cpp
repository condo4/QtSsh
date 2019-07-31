#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>
#include <QTcpSocket>
#include <QEventLoop>
#include <cerrno>
#include <QTime>

Q_LOGGING_CATEGORY(logsshtunnelin, "ssh.tunnelin", QtWarningMsg)

#define BUFFER_LEN (16384)


SshTunnelIn::SshTunnelIn(SshClient *client, const QString &portIdentifier, quint16 localport, quint16 remoteport, QString host)
    : SshChannel(portIdentifier, client)
    , m_localTcpPort(localport)
    , m_remoteTcpPort(remoteport)
    , m_sshListener(nullptr)
    , m_port(localport)
    , m_tcpsocket(nullptr)
    , m_valid(false)
    , m_workinprogress(false)
    , m_needToDisconnect(false)
    , m_needToSendEOF(false)
    , m_tcpBuffer(BUFFER_LEN, 0)
    , m_sshBuffer(BUFFER_LEN, 0)
{
    qCDebug(logsshtunnelin) << m_name << "Try reverse forwarding port" << m_port << "from" << remoteport;

    /* There is an unknown issue here, sometime qssh2_channel_forward_listen_ex will failed with error LIBSSH2_ERROR_REQUEST_DENIED
     * for an unknown reason, if we retry it will do the job... so try a few time
     */
    int retryListen = 10;
    do
    {
        m_sshListener = qssh2_channel_forward_listen_ex(m_sshClient->session(), qPrintable(host), m_remoteTcpPort, &m_remoteTcpPort);
        if (m_sshListener == nullptr)
        {
            int ret = libssh2_session_last_error(m_sshClient->session(), nullptr, nullptr, 0);
            if ( ret==LIBSSH2_ERROR_REQUEST_DENIED && retryListen > 0 )
            {
                retryListen--;
                QTime timer;
                timer.start();
                while(timer.elapsed() < 100)
                {
                    QCoreApplication::processEvents();
                }
            }
            else
            {
                qCCritical(logsshtunnelin, "ERROR : Can't create remote connection throw %s for port %i (error %i)", qPrintable(m_sshClient->getName()) , localport, ret);
                return;
            }
        }
    } while (m_sshListener == nullptr);

    if(remoteport == 0)
    {
        qCDebug(logsshtunnelin) << m_name << " Use dynamic remote port: " << m_remoteTcpPort;
    }

    qCInfo(logsshtunnelin, "INFO : Channel open for %s", qPrintable(m_name));

    QObject::connect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived, Qt::QueuedConnection);
    m_valid = true;
}

SshTunnelIn::~SshTunnelIn()
{
    QObject::disconnect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
}

bool SshTunnelIn::valid() const
{
    return m_valid;
}

quint16 SshTunnelIn::localPort()
{
    return static_cast<unsigned short>(m_localTcpPort);
}

quint16 SshTunnelIn::remotePort()
{
    return static_cast<unsigned short>(m_remoteTcpPort);
}

void SshTunnelIn::onLocalSocketDisconnected()
{
    if(m_workinprogress) {
        qCDebug(logsshtunnelin, "SshTunnelIn::onLocalSocketDisconnected() postponed");
        m_needToDisconnect = true;
        return;
    }

    qCDebug(logsshtunnelin, "SshTunnelIn::onLocalSocketDisconnected() Do Work");
    m_needToDisconnect = false;
    if (!m_tcpsocket.isNull())
    {
        if(m_tcpsocket->state() == QAbstractSocket::ConnectedState)
            m_tcpsocket->disconnectFromHost();
        if(m_tcpsocket->state() == QAbstractSocket::ConnectedState)
            m_tcpsocket->waitForDisconnected();
        QObject::disconnect(m_tcpsocket.data());
        m_tcpsocket.reset();

        if (m_sshChannel != nullptr)
        {
            qCDebug(logsshtunnelin, "SshTunnelIn::onLocalSocketDisconnected() Free Channel");
            qssh2_channel_flush(m_sshChannel);
            free();
        }
    }
}

void SshTunnelIn::onLocalSocketConnected()
{
    qCDebug(logsshtunnelin, "-> forward socket connected and ready");
    QObject::connect(m_tcpsocket.data(), &QTcpSocket::disconnected, this, &SshTunnelIn::onLocalSocketDisconnected, Qt::QueuedConnection);
    QObject::connect(m_tcpsocket.data(), &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
    QObject::connect(m_tcpsocket.data(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onLocalSocketError(QAbstractSocket::SocketError)));

    sshDataReceived();
}

void SshTunnelIn::onLocalSocketError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError)
    {
        if(m_workinprogress)
        {
            m_needToSendEOF = true;
        }
        else
        {
            qCDebug(logsshtunnelin, "SshTunnelIn::onLocalSocketError() SEND EOF");
            qssh2_channel_send_eof(m_sshChannel);
        }
        return;
    }
    qCWarning(logsshtunnelin, "ERROR : SshTunnelIn(%s) : redirection reverse socket error=%i", qPrintable(m_name), error);
}

void SshTunnelIn::onLocalSocketDataReceived()
{
    QObject::disconnect(m_tcpsocket.data(), &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
    qint64 len = 0;
    m_workinprogress = true;

    while( m_tcpsocket->bytesAvailable() > 0)
    {
        /* Read data from local socket */
        qint64 wr  = 0;
        qint64 i   = 0;
        len = m_tcpsocket->read(m_tcpBuffer.data(), m_tcpBuffer.size());
        //qCDebug(logsshtunnelin) <<  "Read from tcp" << len << "total" << m_readByteOnTcp;
        while(wr < len)
        {
            i = qssh2_channel_write(m_sshChannel, m_tcpBuffer.mid(static_cast<int>(wr)).constData(), static_cast<size_t>(len - wr));
            if (i < 0)
            {
                qCWarning(logsshtunnelin, "ERROR : %s  remote failed to write (%lli)", qPrintable(m_name), i);
                break;
            }
            wr += i;
            //qCDebug(logsshtunnelin) <<  "Write into ssh" << i;
        }
    }

    m_workinprogress = false;
    if(m_needToSendEOF) {
        qssh2_channel_send_eof(m_sshChannel);
        m_needToSendEOF = false;
    }
    if(m_needToDisconnect) onLocalSocketDisconnected();
    QObject::connect(m_tcpsocket.data(), &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
}



void SshTunnelIn::sshDataReceived()
{
    QObject::disconnect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
    m_workinprogress = true;
    ssize_t len;
    int ret;

    if (m_sshChannel == nullptr)
    {
        if(!m_tcpsocket.isNull()) qCWarning(logsshtunnelin, "ERROR: tcpsocet allready here");

        connectChannel(libssh2_channel_forward_accept(m_sshListener));
        if(m_sshChannel == nullptr)
        {
            ret = libssh2_session_last_error(m_sshClient->session(), nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                goto exit;
            }
        }
        qCDebug(logsshtunnelin, "sshDataReceived() NEW CONNECTION Create channel");
        qCDebug(logsshtunnelin, "-> forward connection accepted, now create forward socket");

        m_tcpsocket.reset(new QTcpSocket(this));
        //m_tcpsocket->setReadBufferSize(16384);
        QObject::connect(m_tcpsocket.data(), &QTcpSocket::connected, this, &SshTunnelIn::onLocalSocketConnected);

        qCDebug(logsshtunnelin, "-> try to connect forward socket");
        m_tcpsocket->connectToHost(QHostAddress("127.0.0.1"), static_cast<quint16>(m_localTcpPort), QIODevice::ReadWrite);
        goto exit;
    }

    do
    {
        qint64 i;
        ssize_t wr = 0;
        /* Read data from SSH */
        len = libssh2_channel_read_ex(m_sshChannel, 0, m_sshBuffer.data(), static_cast<size_t>(m_sshBuffer.size()));
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            goto exit;
        }
        //qCDebug(logsshtunnelin) <<  "Read from ssh" << len << "total" << m_readByteOnSsh;
        /* Write data into output local socket */
        wr = 0;
        while (wr < len)
        {
            if (m_tcpsocket->isValid() && m_tcpsocket->state() == QAbstractSocket::ConnectedState)
            {
                i = m_tcpsocket->write(m_sshBuffer.mid(static_cast<int>(wr)).constData(), len - wr);
                if (i < 0)
                {
                    qCWarning(logsshtunnelin) << "ERROR : local failed to write (%lli)" << i << wr << len;
                    goto exit;
                }
                else if ( i == 0 )
                {
                    qCWarning(logsshtunnelin) << "TCP socket is full! Waiting" << m_tcpsocket->bytesToWrite() << len << wr;
                    QCoreApplication::processEvents();
                }
                wr += i;
                m_tcpsocket->flush();
            }
        }
    }
    while(len > 0);

    if (libssh2_channel_eof(m_sshChannel))
    {
        qCDebug(logsshtunnelin, "-> Received EOF");
        m_tcpsocket->disconnectFromHost();
    }

exit:
    m_workinprogress = false;
    if(m_needToDisconnect) onLocalSocketDisconnected();
    QObject::connect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived, Qt::QueuedConnection);
}

void SshTunnelIn::close()
{
    if(m_sshListener)
    {
        libssh2_channel_forward_cancel(m_sshListener);
        m_sshListener = nullptr;
    }
}
