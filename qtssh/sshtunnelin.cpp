#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>

#include <QEventLoop>

#define BUFFER_LEN (16384)

//#define DEBUG_SSHCHANNEL

bool SshTunnelIn::valid() const
{
    return _valid;
}

SshTunnelIn::SshTunnelIn(SshClient *client, QString port_identifier, quint16 port, quint16 bind):
    SshChannel(client),
    _localTcpPort(bind),
    _sshListener(nullptr),
    _port(port),
    _name(port_identifier),
    _tcpsocket(nullptr),
    _valid(false)
{
    if(bind == 0)
    {
        qDebug() << "ERROR : " << _name << " Fail to create channel";
        return;
    }

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : try reverse forwarding port " << _port << " from " << bind;
#endif

    int bindport = _localTcpPort;
    _sshListener = qssh2_channel_forward_listen_ex(sshClient->session(), "localhost", _port, &bindport, 1);
    if (_sshListener == nullptr)
    {
        int ret = qssh2_session_last_error(sshClient->session(), nullptr, nullptr, 0);
        qDebug() << "ERROR : Can't create remote connection throw " << sshClient->getName() << " for port " << port << " ( error " << ret << ")";
        return;
    }

    qDebug() << "INFO : Channel open for " << _name;

    sshChannel = qssh2_channel_forward_accept(sshClient->session(), _sshListener);
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

    _tcpsocket = new QTcpSocket(this);
    _tcpsocket->setReadBufferSize(16384);

    QEventLoop wait;
    QTimer timeout;
    bool connected;
    auto con1 = QObject::connect(_tcpsocket, &QTcpSocket::connected, [&connected, &wait]()
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
    auto con3 = QObject::connect(_tcpsocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&connected, &wait]()
    {
        qDebug() << "WARNING : TunnelIn() : Socket Connection Error";
        connected = false;
        wait.quit();
    });
    timeout.start(10000); /* Timeout 10s */
    _tcpsocket->connectToHost(QHostAddress("127.0.0.1"), _localTcpPort, QIODevice::ReadWrite);
    wait.exec();
    QObject::disconnect(con1);
    QObject::disconnect(con2);
    QObject::disconnect(con3);

    if(connected)
    {
        QObject::connect(_tcpsocket, &QTcpSocket::disconnected, this, &SshTunnelIn::onLocalSocketDisconnected);
        QObject::connect(_tcpsocket, &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
        QObject::connect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onLocalSocketError(QAbstractSocket::SocketError)));

    #if defined(DEBUG_SSHCHANNEL)
        qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : onReverseChannelAccepted( 4)" << _tcpsocket;
    #endif
        _valid = true;
    }
    else
    {
        delete _tcpsocket;
        _tcpsocket = nullptr;
    }
}

SshTunnelIn::~SshTunnelIn()
{
    delete _tcpsocket;
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, this, &SshTunnelIn::sshDataReceived);
}

quint16 SshTunnelIn::localPort()
{
    return _localTcpPort;
}

void SshTunnelIn::onLocalSocketDisconnected()
{
    if (_tcpsocket != nullptr)
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
        if (_tcpsocket != nullptr)
        {
            _tcpsocket->disconnectFromHost();
        }
       return;
    }
    qDebug() << "ERROR : SshTunnelIn(" << _name << ") : redirection reverse socket error=" << error;
}

void SshTunnelIn::onLocalSocketDataReceived()
{
    QByteArray buffer(BUFFER_LEN, 0);
    qint64 len = 0;
    qint64 wr  = 0;
    qint64 i   = 0;

    if (_tcpsocket == nullptr)
    {
        qDebug() << "WARNING : Channel " << _name << " : received data when busy";
        return;
    }

    do
    {
        /* Read data from local socket */
        len = _tcpsocket->read(buffer.data(), buffer.size());
        if (-EAGAIN == len)
        {
            break;
        }
        else if (len < 0)
        {
            qDebug() << "ERROR : " << _name << " local failed to read (" << len << ")";
            return;
        }

        do
        {
            i = qssh2_channel_write(sshChannel, buffer.data(), static_cast<size_t>(len));
            if (i < 0)
            {
                qDebug() << "ERROR : " << _name << " remote failed to write (" << i << ")";
                return;
            }
            qssh2_channel_flush(sshChannel);
            sshClient->loopWhileBytesWritten(1000000);
            wr += i;
        } while(wr < len);
    }
    while(_tcpsocket->bytesAvailable() > 0);
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
            qDebug() << "ERROR : " << _name << " remote failed to read (" << len << ")";
            return;
        }

        /* Write data into output local socket */
        if (_tcpsocket == nullptr)
        {
            qDebug() << "ERROR : TCP NULL";
            return;
        }

        wr = 0;
        while (wr < len)
        {
            if (_tcpsocket->isValid() && _tcpsocket->state() == QAbstractSocket::ConnectedState)
            {
                i = _tcpsocket->write(buffer.data() + wr, static_cast<int>(len - wr));
                if (i == -EAGAIN)
                {
                    continue;
                }
                if (i <= 0)
                {
                    qDebug() << "ERROR : " << _name << " local failed to write (" << i << ")";
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
        if(_tcpsocket)
        {
            QEventLoop wait;
            QTimer timeout;
            bool connected;
            QObject::connect(_tcpsocket, &QTcpSocket::disconnected, [&connected, &wait]()
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
            QObject::connect(_tcpsocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&connected, &wait]()
            {
                qDebug() << "WARNING : TunnelIn() : Socket Disconnection Error";
                connected = false;
                wait.quit();
            });
            timeout.start(10000); /* Timeout 10s */
            _tcpsocket->connectToHost(QHostAddress("127.0.0.1"), _localTcpPort, QIODevice::ReadWrite);
            wait.exec();
            _tcpsocket->disconnectFromHost();
            wait.exec();
            _tcpsocket->close();
        }
    }
}
