#include "sshtunnelout.h"
#include "sshclient.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>

SshTunnelOut::SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, QString port_identifier, quint16 port, QObject *parent):
    QObject(parent),
    _opened(false),
    _port(port),
    _name(port_identifier),
    _tcpsocket(tcpSocket),
    _client(client),
    _sshChannel(nullptr),
    _dataSsh(16384, 0),
    _dataSocket(16384, 0),
    _callDepth(0)
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOut::SshTunnelOut() " << _name;
#endif

    _sshChannel = qssh2_channel_direct_tcpip(_client->session(), "127.0.0.1", _port);
    if(_sshChannel == nullptr)
    {
        int ret = qssh2_session_last_error(_client->session(), nullptr, nullptr, 0);
        if(ret != LIBSSH2_ERROR_CHANNEL_FAILURE)
        {
            qDebug() << "ERROR: Can't connect direct tcpip " << ret << " for port " << _port;
        }
#if defined(DEBUG_SSHCHANNEL)
        else
        {
            qDebug() << "DEBUG: Can't connect direct tcpip " << ret << " for port " << _port;
        }
#endif
        return;
    }

    QObject::connect(_client,    &SshClient::sshDataReceived, this, &SshTunnelOut::sshDataReceived, Qt::QueuedConnection);
    QObject::connect(_tcpsocket, &QTcpSocket::readyRead,      this, &SshTunnelOut::tcpDataReceived);
    QObject::connect(_tcpsocket, &QTcpSocket::disconnected,   this, &SshTunnelOut::tcpDisconnected);
    QObject::connect(_tcpsocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &SshTunnelOut::displayError);

    _opened = true;

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOut::SshTunnelOut() OK " << _name;
#endif
    tcpDataReceived();
}

SshTunnelOut::~SshTunnelOut()
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : ~SshTunnelOut() " << _name;
#endif
    _stopSocket();
    _stopChannel();
}

QString SshTunnelOut::name() const
{
    return _name;
}

void SshTunnelOut::sshDataReceived()
{
    ssize_t len = 0,wr = 0;
    int i;

    do
    {
        /* Read data from SSH */
        /*
         * In this case, we must not used qssh2_channel_read
         * beacause we don't need to ProcessEvent to be locked
         * We can return, we will be recall at the next sshDataReceived
         */
        len = libssh2_channel_read(_sshChannel, _dataSsh.data(), _dataSsh.size());
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            return;
        }
        if (len < 0)
        {
            qDebug() << "ERROR : " << _name << " remote failed to read (" << len << ")";
            return;
        }
        if(len == 0)
            return;

        /* Write data into output local socket */
        wr = 0;
        if(_tcpsocket != nullptr)
        {
            while (wr < len)
            {
                i = _tcpsocket->write( _dataSsh.mid(wr, len));
                if (i <= 0)
                {
                    qDebug() << "ERROR : " << _name << " local failed to write (" << i << ")";
                    return;
                }
                wr += i;
            }
        }
        else
        {
            if(_opened) qDebug() << "ERROR : Data loose";
        }

        if (qssh2_channel_eof(_sshChannel) && _opened)
        {
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : Disconnected from ssh";
#endif
            emit disconnected();
        }
    }
    while(len == _dataSsh.size());
}

void SshTunnelOut::tcpDataReceived()
{
    qint64 len = 0;
    ssize_t wr = 0;
    ssize_t i = 0;

    if (_tcpsocket == nullptr || _sshChannel == nullptr)
    {
        qDebug() << "ERROR : SshTunnelOut(" << _name << ") : received TCP data but not seems to be a valid Tcp socket or channel not ready";
        return;
    }

    do
    {
        /* Read data from local socket */
        len = _tcpsocket->read(_dataSocket.data(), _dataSocket.size());
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
            i = qssh2_channel_write(_sshChannel, _dataSocket.data(), static_cast<quint64>(len));
            if (i < 0)
            {
                qDebug() << "ERROR : " << _name << " remote failed to write (" << i << ")";
                return;
            }
            wr += i;
        } while(i > 0 && wr < len);
    }
    while(len > 0);
}

void SshTunnelOut::tcpDisconnected()
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : Disconnected from socket";
#endif
    emit disconnected();
}

bool SshTunnelOut::ready() const
{
    return _opened;
}

void SshTunnelOut::displayError(QAbstractSocket::SocketError error)
{
    if(error != QAbstractSocket::RemoteHostClosedError)
    qDebug() << "ERROR : SshTunnelOut(" << _name << ") : redirection socket error=" << error;
}
void SshTunnelOut::_stopChannel()
{
    if (_sshChannel == nullptr)
        return;

    QObject::disconnect(_client,    &SshClient::sshDataReceived, this, &SshTunnelOut::sshDataReceived);

    int ret = qssh2_channel_close(_sshChannel);

    if(ret)
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshChannel() : Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
#endif
        return;
    }

    ret = qssh2_channel_wait_closed(_sshChannel);
    if(ret)
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshChannel() : Failed to channel_wait_closed";
#endif
        return;
    }

    ret = qssh2_channel_free(_sshChannel);
    if(ret)
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshChannel() : Failed to channel_free";
#endif
        return;
    }
    _sshChannel = nullptr;
}

void SshTunnelOut::_stopSocket()
{
    if(_tcpsocket)
    {
        QObject::disconnect(_tcpsocket, &QTcpSocket::readyRead, this,  &SshTunnelOut::tcpDataReceived);
        QObject::disconnect(_tcpsocket, SIGNAL(disconnected()),                        this, SLOT(tcpDisconnected()));
        QObject::disconnect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(displayError(QAbstractSocket::SocketError)));
        if(_tcpsocket->state() == QAbstractSocket::ConnectedState)
        {
            _tcpsocket->disconnectFromHost();
        }
        _tcpsocket->deleteLater();
        _tcpsocket = nullptr;
    }
}
