#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>
#include <QTcpSocket>
#include <QEventLoop>
#include <cerrno>

Q_LOGGING_CATEGORY(logsshtunnelin, "ssh.tunnelin", QtWarningMsg)

#define BUFFER_LEN (16384)


SshTunnelIn::SshTunnelIn(SshClient *client, const QString &portIdentifier, quint16 port, quint16 bind, QString host)
    : SshChannel(client)
    , m_localTcpPort(bind)
    , m_sshListener(nullptr)
    , m_port(port)
    , m_name(portIdentifier)
    , m_tcpsocket(nullptr)
    , m_valid(false)
    , m_workinprogress(false)
    , m_needToDisconnect(false)
    , m_needToSendEOF(false)
{
    if(bind == 0)
    {
        qCWarning(logsshtunnelin, "ERROR : %s Fail to create channel", qPrintable(m_name));
        return;
    }

    qCDebug(logsshtunnelin, "Try reverse forwarding port %i from %i (%s)", m_port, bind, qPrintable(m_name));

    int bindport = m_localTcpPort;
    m_sshListener = qssh2_channel_forward_listen_ex(sshClient->session(), qPrintable(host), m_port, &bindport);
    if (m_sshListener == nullptr)
    {
        int ret = qssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
        qCDebug(logsshtunnelin, "ERROR : Can't create remote connection throw %s for port %i (error %i)", qPrintable(sshClient->getName()) , port, ret);
        return;
    }

    qCInfo(logsshtunnelin, "INFO : Channel open for %s", qPrintable(m_name));

    QObject::connect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived, Qt::QueuedConnection);
    m_valid = true;
}

SshTunnelIn::~SshTunnelIn()
{
    delete m_tcpsocket;
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
}

bool SshTunnelIn::valid() const
{
    return m_valid;
}

quint16 SshTunnelIn::localPort()
{
    return m_localTcpPort;
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
    if (m_tcpsocket != nullptr)
    {
        QObject::connect(m_tcpsocket, &QObject::destroyed, [this](){ m_tcpsocket = nullptr; });
        m_tcpsocket->deleteLater();

        if (sshChannel != nullptr)
        {
            qCDebug(logsshtunnelin, "SshTunnelIn::onLocalSocketDisconnected() Free Channel");
            qssh2_channel_flush(sshChannel);
            qssh2_channel_free(sshChannel);
            sshChannel = nullptr;
        }
    }
}

void SshTunnelIn::onLocalSocketConnected()
{
    qCDebug(logsshtunnelin, "-> forward socket connected and ready");
    QObject::connect(m_tcpsocket, &QTcpSocket::disconnected, this, &SshTunnelIn::onLocalSocketDisconnected, Qt::QueuedConnection);
    QObject::connect(m_tcpsocket, &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
    QObject::connect(m_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onLocalSocketError(QAbstractSocket::SocketError)));

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
            qssh2_channel_send_eof(sshChannel);
        }
        return;
    }
    qCWarning(logsshtunnelin, "ERROR : SshTunnelIn(%s) : redirection reverse socket error=%i", qPrintable(m_name), error);
}

void SshTunnelIn::onLocalSocketDataReceived()
{
    QObject::disconnect(m_tcpsocket, &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
    QByteArray buffer(BUFFER_LEN, 0);
    qint64 len = 0;
    qint64 wr  = 0;
    qint64 i   = 0;
    m_workinprogress = true;
    do
    {
        /* Read data from local socket */
        wr = 0;
        len = m_tcpsocket->read(buffer.data(), buffer.size());
        if (-EAGAIN == len)
        {
            QCoreApplication::processEvents();
            break;
        }
        if (len < 0)
        {
            qCWarning(logsshtunnelin, "ERROR : %s local failed to read (%lli)", qPrintable(m_name), len);
            goto exit2;
        }

        do
        {
            i = qssh2_channel_write(sshChannel, buffer.data() + wr, static_cast<size_t>(len - wr));
            if (i < 0)
            {
                qCWarning(logsshtunnelin, "ERROR : %s  remote failed to write (%lli)", qPrintable(m_name), i);
                goto exit2;
            }
            wr += i;
        } while(wr < len);
    }
    while(m_tcpsocket->bytesAvailable() > 0);

exit2:
    m_workinprogress = false;
    if(m_needToSendEOF) {
        qssh2_channel_send_eof(sshChannel);
        m_needToSendEOF = false;
    }
    if(m_needToDisconnect) onLocalSocketDisconnected();
    QObject::connect(m_tcpsocket, &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
}



void SshTunnelIn::sshDataReceived()
{
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
    m_workinprogress = true;
    QByteArray buffer(BUFFER_LEN, 0);
    ssize_t len,wr;
    qint64 i;
    int ret;

    if (sshChannel == nullptr)
    {
        if(m_tcpsocket) qCWarning(logsshtunnelin, "ERROR: tcpsocet allready here");

        sshChannel = libssh2_channel_forward_accept(m_sshListener);
        if(sshChannel == nullptr)
        {
            ret = libssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                goto exit;
            }
        }
        qCDebug(logsshtunnelin, "sshDataReceived() NEW CONNECTION Create channel");
        qCDebug(logsshtunnelin, "-> forward connection accepted, now create forward socket");

        m_tcpsocket = new QTcpSocket(this);
        m_tcpsocket->setReadBufferSize(16384);
        QObject::connect(m_tcpsocket, &QTcpSocket::connected, this, &SshTunnelIn::onLocalSocketConnected);

        qCDebug(logsshtunnelin, "-> try to connect forward socket");
        m_tcpsocket->connectToHost(QHostAddress("127.0.0.1"), m_localTcpPort, QIODevice::ReadWrite);
        goto exit;
    }

    do
    {
        /* Read data from SSH */
        len = libssh2_channel_read_ex(sshChannel, 0, buffer.data(), static_cast<size_t>(buffer.size()));
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            goto exit;
        }

        /* Write data into output local socket */
        wr = 0;
        while (wr < len)
        {
            if (m_tcpsocket->isValid() && m_tcpsocket->state() == QAbstractSocket::ConnectedState)
            {
                i = m_tcpsocket->write(buffer.data() + wr, static_cast<int>(len - wr));
                if (i == -EAGAIN)
                {
                    continue;
                }
                if (i <= 0)
                {
                    qCWarning(logsshtunnelin, "ERROR : local failed to write (%lli)", i);
                    goto exit;
                }
                wr += i;
                m_tcpsocket->flush();
            }
        }
    }
    while(len > 0);

    if (qssh2_channel_eof(sshChannel))
    {
        qCDebug(logsshtunnelin, "-> Received EOF");
        m_tcpsocket->disconnectFromHost();
    }

exit:
    m_workinprogress = false;
    if(m_needToDisconnect) onLocalSocketDisconnected();
    QObject::connect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived, Qt::QueuedConnection);
}
