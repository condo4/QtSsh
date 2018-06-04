#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>
#include <QTcpSocket>
#include "errno.h"

#include <QEventLoop>

#define BUFFER_LEN (16384)

//#define DEBUG_SSHCHANNEL

bool SshTunnelIn::valid() const
{
    return m_valid;
}

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
        qDebug() << "ERROR : " << m_name << " Fail to create channel";
        return;
    }

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : try reverse forwarding port " << _port << " from " << bind;
#endif

    int bindport = m_localTcpPort;
    m_sshListener = qssh2_channel_forward_listen_ex(sshClient->session(), "localhost", m_port, &bindport, 1);
    if (m_sshListener == nullptr)
    {
        int ret = qssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
        qDebug() << "ERROR : Can't create remote connection throw " << sshClient->getName() << " for port " << port << " ( error " << ret << ")";
        return;
    }

    qDebug() << "INFO : Channel open for " << m_name;

    sshChannel = qssh2_channel_forward_accept(sshClient->session(), m_sshListener);
    if (sshChannel == nullptr)
    {
        int ret = qssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
        qDebug() << "ERROR : Can't accept forward (" << ret << ")";
        return;
    }
    QObject::connect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived, Qt::QueuedConnection);


#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelIn(" << _name << ":" << _port << " @" << this <<") : onReverseChannelAccepted()";
#endif

    m_tcpsocket = new QTcpSocket(this);
    m_tcpsocket->setReadBufferSize(16384);

    QEventLoop wait;
    QTimer timeout;
    bool connected;
    auto con1 = QObject::connect(m_tcpsocket, &QTcpSocket::connected, [&connected, &wait]()
    {
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelIn() : Socket Connected";
#endif
        connected = true;
        wait.quit();
    });
    auto con2 = QObject::connect(&timeout, &QTimer::timeout, [&connected, &wait]()
    {
        qDebug() << "WARNING : TunnelIn() : Socket Connection Timeout";
        connected = false;
        wait.quit();
    });
    auto con3 = QObject::connect(m_tcpsocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&connected, &wait]()
    {
        qDebug() << "WARNING : TunnelIn() : Socket Connection Error";
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

    #if defined(DEBUG_SSHCHANNEL)
        qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : onReverseChannelAccepted( 4)" << _tcpsocket;
    #endif
        m_valid = true;
    }
    else
    {
        delete m_tcpsocket;
        m_tcpsocket = nullptr;
    }
}

SshTunnelIn::~SshTunnelIn()
{
    delete m_tcpsocket;
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
}

quint16 SshTunnelIn::localPort()
{
    return m_localTcpPort;
}

void SshTunnelIn::onLocalSocketDisconnected()
{
    if (m_tcpsocket != nullptr)
    {
        if (sshChannel != nullptr)
        {
            qssh2_channel_send_eof(sshChannel);
        }
#if defined(DEBUG_SSHCHANNEL)
        qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : tcp reverse socket disconnected !";
#endif
    }
}

void SshTunnelIn::onLocalSocketError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError)
    {
        if (m_tcpsocket != nullptr)
        {
            m_tcpsocket->disconnectFromHost();
        }
       return;
    }
    qDebug() << "ERROR : SshTunnelIn(" << m_name << ") : redirection reverse socket error=" << error;
}

void SshTunnelIn::onLocalSocketDataReceived()
{
    QByteArray buffer(BUFFER_LEN, 0);
    qint64 len = 0;
    qint64 wr  = 0;
    qint64 i   = 0;

    if (m_tcpsocket == nullptr)
    {
        qDebug() << "WARNING : Channel " << m_name << " : received data when busy";
        return;
    }

    do
    {
        /* Read data from local socket */
        len = m_tcpsocket->read(buffer.data(), buffer.size());
        if (-EAGAIN == len)
        {
            break;
        }
        else if (len < 0)
        {
            qDebug() << "ERROR : " << m_name << " local failed to read (" << len << ")";
            return;
        }

        do
        {
            i = qssh2_channel_write(sshChannel, buffer.data(), static_cast<size_t>(len));
            if (i < 0)
            {
                qDebug() << "ERROR : " << m_name << " remote failed to write (" << i << ")";
                return;
            }
            qssh2_channel_flush(sshChannel);
            sshClient->loopWhileBytesWritten(1000000);
            wr += i;
        } while(wr < len);
    }
    while(m_tcpsocket->bytesAvailable() > 0);
}

void SshTunnelIn::sshDataReceived()
{
    QByteArray buffer(BUFFER_LEN, 0);
    ssize_t len,wr;
    qint64 i;

    if (sshChannel == nullptr)
    {
        qDebug() << "WARNING : Received data on non existing channel";
        return;
    }

    do
    {
        /* Read data from SSH */
        len = qssh2_channel_read(sshChannel, buffer.data(), static_cast<size_t>(buffer.size()));
        if (len < 0)
        {
            qDebug() << "ERROR : " << m_name << " remote failed to read (" << len << ")";
            return;
        }

        /* Write data into output local socket */
        if (m_tcpsocket == nullptr)
        {
            qDebug() << "ERROR : TCP NULL";
            return;
        }

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
                    qDebug() << "ERROR : " << m_name << " local failed to write (" << i << ")";
                    return;
                }
                wr += i;
            }
        }
    }
    while(len > 0);

    if (qssh2_channel_eof(sshChannel))
    {
#if defined(DEBUG_SSHCHANNEL)
        qDebug() << "DEBUG : Disconnect channel";
#endif
        if(m_tcpsocket)
        {
            QEventLoop wait;
            QTimer timeout;
            bool connected;
            QObject::connect(m_tcpsocket, &QTcpSocket::disconnected, [&connected, &wait]()
            {
        #if defined(DEBUG_SSHCHANNEL)
            qDebug() << "DEBUG : SshTunnelIn() : Socket Disconnected";
        #endif
                connected = true;
                wait.quit();
            });
            QObject::connect(&timeout, &QTimer::timeout, [&connected, &wait]()
            {
                qDebug() << "WARNING : TunnelIn() : Socket Disconnection Timeout";
                connected = false;
                wait.quit();
            });
            QObject::connect(m_tcpsocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&connected, &wait]()
            {
                qDebug() << "WARNING : TunnelIn() : Socket Disconnection Error";
                connected = false;
                wait.quit();
            });
            timeout.start(10000); /* Timeout 10s */
            m_tcpsocket->connectToHost(QHostAddress("127.0.0.1"), m_localTcpPort, QIODevice::ReadWrite);
            wait.exec();
            m_tcpsocket->disconnectFromHost();
            wait.exec();
            m_tcpsocket->close();
        }
    }
}
