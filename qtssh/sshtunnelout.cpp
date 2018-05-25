#include "sshtunnelout.h"
#include "sshclient.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>

SshTunnelOut::SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, QString port_identifier, quint16 port):
    QObject(client),
    _opened(false),
    _port(port),
    _name(port_identifier),
    _tcpsocket(tcpSocket),
    _client(client),
    _sshChannel(nullptr),
    _dataSsh(16384, 0),
    _dataSocket(16384, 0)
{
    _sshChannel = qssh2_channel_direct_tcpip(_client->session(), "127.0.0.1", _port);
    if(_sshChannel == nullptr)
    {
        int ret = qssh2_session_last_error(_client->session(), nullptr, nullptr, 0);
        if(ret != LIBSSH2_ERROR_CHANNEL_FAILURE)
        {
            qDebug() << "ERROR: Can't connect direct tcpip " << ret << " for port " << _port;
        }
#if defined(DEBUG_SSHCLIENT)
        else
        {
            qDebug() << "DEBUG: Can't connect direct tcpip " << ret << " for port " << _port;
        }
#endif
        return;
    }

    QObject::connect(_tcpsocket, &QTcpSocket::readyRead,      this, &SshTunnelOut::tcpDataReceived);
    QObject::connect(_tcpsocket, &QTcpSocket::disconnected,   this, &SshTunnelOut::tcpDisconnected);
    QObject::connect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(displayError(QAbstractSocket::SocketError)));

#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshTunnelOut : Connection" << port_identifier << "created";
#endif
    _opened = true;

    _sshChannel = qssh2_channel_direct_tcpip(_client->session(), "127.0.0.1", _port);
    if (_sshChannel == nullptr)
    {
         char *errmsg;
        int errlen;
        int err = qssh2_session_last_error(_client->session(), &errmsg, &errlen, 0);

        qDebug() << "ERROR : SshTunnelOut(" << _name << ") : direct_tcpip failed :" << err << QString::fromLocal8Bit(errmsg, errlen);
    }
}

QString SshTunnelOut::name() const
{
    return _name;
}


void SshTunnelOut::close(QString reason)
{
    if(!_opened) return;
    _opened = false;
    if(_tcpsocket)
    {
        QObject::disconnect(_tcpsocket, &QTcpSocket::readyRead, this,  &SshTunnelOut::tcpDataReceived);
        QObject::disconnect(_tcpsocket, SIGNAL(disconnected()),                        this, SLOT(tcpDisconnected()));
        QObject::disconnect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(displayError(QAbstractSocket::SocketError)));
        if(_tcpsocket->state() == QAbstractSocket::ConnectedState)
            _tcpsocket->disconnectFromHost();
        _tcpsocket->deleteLater();
        _tcpsocket = nullptr;
    }
    if(_sshChannel) qssh2_channel_free(_sshChannel);
#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : Connection" << _name << "closed (" << reason << ")";
#else
    Q_UNUSED(reason);
#endif
    emit disconnected();
}

void SshTunnelOut::sshDataReceived()
{
    ssize_t len = 0,wr = 0;
    int i;

    do
    {
        /* Read data from SSH */
        len = qssh2_channel_read(_sshChannel, _dataSsh.data(), _dataSsh.size());
        if (len < 0)
        {
            qDebug() << "ERROR : " << _name << " remote failed to read (" << len << ")";
            return;
        }

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
            close("channel_eof");
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
            i = qssh2_channel_write(_sshChannel, _dataSocket.data(), len);
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
    close("socket_diconnected");
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
