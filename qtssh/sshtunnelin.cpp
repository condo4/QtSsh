#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>
#include <QTcpSocket>
#include "errno.h"

#include <QEventLoop>

Q_LOGGING_CATEGORY(logsshtunnelin, "ssh.tunnelin", QtWarningMsg)

#define BUFFER_LEN (16384)


SshTunnelIn::SshTunnelIn(SshClient *client, QString port_identifier, quint16 port, quint16 bind):
    SshChannel(client),
    m_localTcpPort(bind),
    m_sshListener(nullptr),
    m_port(port),
    m_name(port_identifier),
    m_tcpsocket(nullptr),
    m_valid(false)
{
    if(bind == 0)
    {
        qCWarning(logsshtunnelin) << "ERROR : " << m_name << " Fail to create channel";
        return;
    }

    qCDebug(logsshtunnelin) << "SshTunnelIn(" << m_name << ") : try reverse forwarding port " << m_port << " from " << bind;

    int bindport = m_localTcpPort;
    m_sshListener = qssh2_channel_forward_listen_ex(sshClient->session(), "localhost", m_port, &bindport, 1);
    if (m_sshListener == nullptr)
    {
        int ret = qssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
        qCDebug(logsshtunnelin) << "ERROR : Can't create remote connection throw " << sshClient->getName() << " for port " << port << " ( error " << ret << ")";
        return;
    }

    qCInfo(logsshtunnelin) << "INFO : Channel open for " << m_name;

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
    qCDebug(logsshtunnelin) << "SshTunnelIn::onLocalSocketDisconnected()";
    if (m_tcpsocket != nullptr)
    {
        if (sshChannel != nullptr)
        {
            qssh2_channel_send_eof(sshChannel);
        }
        qCDebug(logsshtunnelin) << "SshTunnelIn(" << m_name << ") : tcp reverse socket disconnected !";
    }
}

void SshTunnelIn::onLocalSocketError(QAbstractSocket::SocketError error)
{
    qCDebug(logsshtunnelin) << "SshTunnelIn::onLocalSocketError()";
    if (error == QAbstractSocket::RemoteHostClosedError)
    {
        if (m_tcpsocket != nullptr)
        {
            m_tcpsocket->disconnectFromHost();
        }
       return;
    }
    qCWarning(logsshtunnelin) << "ERROR : SshTunnelIn(" << m_name << ") : redirection reverse socket error=" << error;
}

void SshTunnelIn::onLocalSocketDataReceived()
{
    qCDebug(logsshtunnelin) << "SshTunnelIn::onLocalSocketDataReceived()";
    QByteArray buffer(BUFFER_LEN, 0);
    qint64 len = 0;
    qint64 wr  = 0;
    qint64 i   = 0;

    if (m_tcpsocket == nullptr)
    {
        qCWarning(logsshtunnelin) << "WARNING : Channel " << m_name << " : received data when busy";
        return;
    }

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
        else if (len < 0)
        {
            qCWarning(logsshtunnelin) << "ERROR : " << m_name << " local failed to read (" << len << ")";
            return;
        }

        do
        {
            i = qssh2_channel_write(sshChannel, buffer.data() + wr, static_cast<size_t>(len - wr));
            if (i < 0)
            {
                qCWarning(logsshtunnelin) << "ERROR : " << m_name << " remote failed to write (" << i << ")";
                return;
            }
            qssh2_channel_flush(sshChannel);
            wr += i;
        } while(wr < len);
    }
    while(m_tcpsocket->bytesAvailable() > 0);
}

void SshTunnelIn::sshDataReceived()
{
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
    QByteArray buffer(BUFFER_LEN, 0);
    ssize_t len,wr;
    qint64 i;
    int ret;

    if (sshChannel == nullptr)
    {
        sshChannel = libssh2_channel_forward_accept(m_sshListener);
        if(sshChannel == nullptr)
        {
            ret = libssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                goto exit;
            }
        }
        qCDebug(logsshtunnelin) << "Accept forward connection";
    }

    if(m_tcpsocket == nullptr)
    {
        qCDebug(logsshtunnelin) << "Create forward socket";
        m_tcpsocket = new QTcpSocket(this);
        m_tcpsocket->setReadBufferSize(16384);

        QEventLoop wait;
        QTimer timeout;
        bool connected;
        auto con1 = QObject::connect(m_tcpsocket, &QTcpSocket::connected, [&connected, &wait]()
        {
            qCDebug(logsshtunnelin) << "Forward socket Connected";
            connected = true;
            wait.quit();
        });
        auto con2 = QObject::connect(&timeout, &QTimer::timeout, [&connected, &wait]()
        {
            qCWarning(logsshtunnelin) << "Forward socket Connection Timeout";
            connected = false;
            wait.quit();
        });
        auto con3 = QObject::connect(m_tcpsocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&connected, &wait]()
        {
            qCWarning(logsshtunnelin) << "Forward socket Connection Error";
            connected = false;
            wait.quit();
        });
        timeout.start(10000); /* Timeout 10s */
        m_tcpsocket->connectToHost(QHostAddress("127.0.0.1"), m_localTcpPort, QIODevice::ReadWrite);
        wait.exec();
        QObject::disconnect(con1);
        QObject::disconnect(con2);
        QObject::disconnect(con3);

        if(connected)
        {
            QObject::connect(m_tcpsocket, &QTcpSocket::disconnected, this, &SshTunnelIn::onLocalSocketDisconnected);
            QObject::connect(m_tcpsocket, &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
            QObject::connect(m_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onLocalSocketError(QAbstractSocket::SocketError)));

            qCDebug(logsshtunnelin) << "SshTunnelIn(" << m_name << ") : onReverseChannelAccepted( 4)" << m_tcpsocket;
        }
        else
        {
            qCWarning(logsshtunnelin) << "SshTunnelIn(" << m_name << ") : Something wrong on connection";
            delete m_tcpsocket;
            m_tcpsocket = nullptr;
            goto exit;
        }
    }

    do
    {
        /* Read data from SSH */
        len = libssh2_channel_read_ex(sshChannel, 0, buffer.data(), static_cast<size_t>(buffer.size()));
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            goto exit;
        }

        if(len)
            qCDebug(logsshtunnelin) << "Recived data from ssh " << buffer.mid(0, static_cast<int>(len));

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
                    qCWarning(logsshtunnelin) << "ERROR : " << m_name << " local failed to write (" << i << ")";
                    goto exit;
                }
                wr += i;
            }
        }
    }
    while(len > 0);

    if (qssh2_channel_eof(sshChannel))
    {
        qCDebug(logsshtunnelin) << "Disconnect channel";
        if(m_tcpsocket)
        {
            QObject::disconnect(m_tcpsocket, &QTcpSocket::disconnected, this, &SshTunnelIn::onLocalSocketDisconnected);
            QEventLoop wait;
            QTimer timeout;
            bool disconnected = false;
            m_tcpsocket->flush();
            auto con1 = QObject::connect(m_tcpsocket, &QTcpSocket::disconnected, [&disconnected, &wait]()
            {
                qCDebug(logsshtunnelin) << "Socket Disconnected";
                disconnected = true;
                wait.quit();
            });
            auto con2 = QObject::connect(&timeout, &QTimer::timeout, [&disconnected, &wait]()
            {
                qCWarning(logsshtunnelin) << "WARNING : TunnelIn() : Socket Disconnection Timeout";
                disconnected = false;
                wait.quit();
            });
            auto con3 = QObject::connect(m_tcpsocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&disconnected, &wait]()
            {
                qCWarning(logsshtunnelin) << "WARNING : TunnelIn() : Socket Disconnection Error";
                disconnected = false;
                wait.quit();
            });
            timeout.start(10000); /* Timeout 10s */
            m_tcpsocket->disconnectFromHost();
            if(!disconnected)
                wait.exec();
            QObject::disconnect(con1);
            QObject::disconnect(con2);
            QObject::disconnect(con3);
            m_tcpsocket->close();
            delete m_tcpsocket;
            m_tcpsocket = nullptr;

            libssh2_channel_free(sshChannel);
            sshChannel = nullptr;
        }
    }

exit:
    QObject::connect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived, Qt::QueuedConnection);
}
