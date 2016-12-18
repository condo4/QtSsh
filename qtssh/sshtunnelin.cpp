#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>

#include <QEventLoop>

#define BUFFER_LEN (16384)

SshTunnelIn::SshTunnelIn(SshClient *client, QString port_identifier, quint16 port, quint16 bind):
    SshChannel(client),
    _localTcpPort(bind),
    _sshListener(NULL),
    _currentState(TunnelListenTcpServer),
    _port(port),
    _name(port_identifier),
    _tcpsocket(NULL)
{
    if(bind == 0)
    {
        qDebug() << "ERROR : " << _name << " Fail to create channel";
        return;
    }

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : try reverse forwarding port " << _port << " from " << bind;
#endif

    sshDataReceived();
}

quint16 SshTunnelIn::localPort()
{
    return _localTcpPort;
}

void SshTunnelIn::onLocalSocketDisconnected()
{
    if (_tcpsocket != NULL)
    {
        if (sshChannel != NULL)
        {
            libssh2_channel_send_eof(sshChannel);
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
        if (_tcpsocket != NULL)
        {
            _tcpsocket->disconnectFromHost();
        }
       return;
    }
    qDebug() << "ERROR : SshTunnelIn(" << _name << ") : redirection reverse socket error=" << error;
}

void SshTunnelIn::onLocalSocketDataReceived()
{
    char buf[BUFFER_LEN];
    qint64 len = 0;
    qint64 wr  = 0;
    qint64 i   = 0;

    if (_tcpsocket == NULL)
    {
        qDebug() << "WARNING : Channel " << _name << " : received data when busy";
        return;
    }

    do
    {
        /* Read data from local socket */
        len = _tcpsocket->read(buf, sizeof(buf));
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
            i = libssh2_channel_write(sshChannel, buf, len);
            if (i == LIBSSH2_ERROR_EAGAIN)
            {
                QTimer timer;
                QEventLoop loop;
                connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
                timer.start(1000);
                loop.exec();
            }
            else
            {
                if (i < 0)
                {
                    qDebug() << "ERROR : " << _name << " remote failed to write (" << i << ")";
                    return;
                }
                libssh2_channel_flush(sshChannel);
                sshClient->waitForBytesWritten(1000000);
                wr += i;
            }
        } while(wr < len);
        if(len > 0) emit data_tx(len);
    }
    while(_tcpsocket->bytesAvailable() > 0);
}

void SshTunnelIn::sshDataReceived()
{
    int bind = _localTcpPort;
    char * errmsg;
    int errlen;
    int err;
    int requestretry = 5;

    switch(_currentState)
    {
    case TunnelListenTcpServer:
        _sshListener = libssh2_channel_forward_listen_ex(sshClient->session(), "localhost", _port, &bind, 1);
        if (_sshListener != NULL)
        {
#if defined(DEBUG_SSHCHANNEL)
            qDebug() << "DEBUG : QtSshChannel ( for " << _name << ") : remote server listen";
#endif
            qDebug() << "INFO : Channel open for " << _name;
            _currentState = TunnelAcceptChannel;
            sshDataReceived();
            return;
        }

        err = libssh2_session_last_error(sshClient->session(), &errmsg, &errlen, 0);
        if(err == LIBSSH2_ERROR_REQUEST_DENIED)
        {
            if(requestretry > 0)
            {
                --requestretry;
                break;
            }
            else
            {
                qDebug() << "ERROR : Channel : last session error (" << err << ") " << errmsg;
                _currentState = TunnelErrorNoRetry;
                break;
            }
        }
        else if (err != LIBSSH2_ERROR_EAGAIN)
        {
            qDebug() << "ERROR : Channel : last session error (" << err << ") " << errmsg;
            _currentState = TunnelErrorNoRetry;
            break;
        }

#if defined(DEBUG_SSHCHANNEL)
        else
        {
            qDebug() << "DEBUG : QtSshChannel ( for " << _name << ")  : ssh listener returned is NULL ! Would wait...";
        }
#endif

        break;

    case TunnelAcceptChannel:
        sshChannel = libssh2_channel_forward_accept(_sshListener);
        if (sshChannel == NULL)
        {
            err = libssh2_session_last_error(sshClient->session(), &errmsg, &errlen, 0);
            if (err == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            else
            {
                this->_currentState = TunnelErrorNoRetry;
                return;
            }
        }
        else
        {
            _currentState = TunnelReadyRead;

#if defined(DEBUG_SSHCHANNEL)
            qDebug() << "DEBUG : SshTunnelIn(" << _name << ":" << _port << " @" << this <<") : onReverseChannelAccepted()";
#endif

            if (_tcpsocket == NULL)
            {
                _tcpsocket = new QTcpSocket(this);
                _tcpsocket->setReadBufferSize(16384);
                QObject::connect(_tcpsocket, &QTcpSocket::disconnected, this, &SshTunnelIn::onLocalSocketDisconnected);
                QObject::connect(_tcpsocket, &QTcpSocket::readyRead,    this, &SshTunnelIn::onLocalSocketDataReceived);
                QObject::connect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onLocalSocketError(QAbstractSocket::SocketError)));
            }

            if (_tcpsocket != NULL)
            {
#if defined(DEBUG_SSHCHANNEL)
                qDebug() << "DEBUG : SshTunnelIn(" << _name << ") : onReverseChannelAccepted( 4)" << _tcpsocket;
#endif
                if (_tcpsocket->state() == QAbstractSocket::UnconnectedState)
                {
                    _tcpsocket->connectToHost(QHostAddress("127.0.0.1"), _localTcpPort);
                }
            }
            return;
        }

    case TunnelReadyRead:
        readSshData();
        break;

    case TunnelErrorNoRetry:
    case TunnelError:
        qDebug() << "ERROR : Tunnel is broken";
        break;

    }
}

void SshTunnelIn::readSshData()
{
    char buf[BUFFER_LEN];
    ssize_t len,wr;
    int i;

    if (sshChannel == NULL)
    {
        qDebug() << "WARNING : Received data on non existing channel";
        return;
    }

    do
    {
        /* Read data from SSH */
        len = libssh2_channel_read(sshChannel, buf, sizeof(buf));

        if (LIBSSH2_ERROR_EAGAIN == len)
        {
            continue;
        }
        else if (len < 0)
        {
            qDebug() << "ERROR : " << _name << " remote failed to read (" << len << ")";
            return;
        }

        /* Write data into output local socket */
        if (_tcpsocket == NULL)
        {
            qDebug() << "ERROR : TCP NULL";
            return;
        }

        wr = 0;
        while (wr < len)
        {
            if (_tcpsocket->isValid() && _tcpsocket->state() == QAbstractSocket::ConnectedState)
            {
                i = _tcpsocket->write(buf + wr, len - wr);
                if (i == -EAGAIN)
                {
                    i = 1;
                    continue;
                }
                if (i <= 0)
                {
                    qDebug() << "ERROR : " << _name << " local failed to write (" << i << ")";
                    return;
                }
                wr += i;
            }
            else if (_tcpsocket->isValid() && _tcpsocket->state() == QAbstractSocket::UnconnectedState)
            {
                qDebug() << "WARNING : Channel " << _name << " : try to reconnect the local tcp socket";
                _tcpsocket->connectToHost(QHostAddress("127.0.0.1"), _localTcpPort, QIODevice::ReadWrite);
                _tcpsocket->waitForConnected(30000);
            }
        }

        emit data_rx(len);

    }
    while(len > 0);

    if (libssh2_channel_eof(sshChannel))
    {
#if defined(DEBUG_SSHCHANNEL)
        qDebug() << "DEBUG : Disconnect channel";
#endif
        _currentState = TunnelAcceptChannel;
        _tcpsocket->disconnectFromHost();
        _tcpsocket->close();
    }
}

