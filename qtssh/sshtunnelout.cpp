#include "sshtunnelout.h"
#include "sshclient.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>

SshTunnelOut::SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, QString port_identifier, quint16 port):
    SshChannel(client),
    _opened(true),
    _port(port),
    _name(port_identifier),
    _tcpsocket(tcpSocket)
{
    sshChannel = libssh2_channel_direct_tcpip(this->sshClient->session(), "127.0.0.1", _port);
    if(sshChannel) emit channelReady();
    QObject::connect(_tcpsocket, &QTcpSocket::readyRead,      this, &SshTunnelOut::tcpDataReceived);
    QObject::connect(_tcpsocket, &QTcpSocket::disconnected,   this, &SshTunnelOut::tcpDisconnected);
    QObject::connect(client,     &SshClient::sshDataReceived, this, &SshTunnelOut::sshDataReceived);
    QObject::connect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(displayError(QAbstractSocket::SocketError)));

#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshTunnelOut : Connection" << port_identifier << "created";
#endif
}

void SshTunnelOut::close(QString reason)
{
    if(!_opened) return;
    _opened = false;
    QObject::disconnect(_tcpsocket, &QTcpSocket::readyRead, this,  &SshTunnelOut::tcpDataReceived);
    QObject::disconnect(_tcpsocket, SIGNAL(disconnected()),                        this, SLOT(tcpDisconnected()));
    QObject::disconnect(sshClient,SIGNAL(sshDataReceived()), this, SLOT(sshDataReceived()));
    QObject::disconnect(_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(displayError(QAbstractSocket::SocketError)));
    if(_tcpsocket->state() == QAbstractSocket::ConnectedState)
        _tcpsocket->disconnectFromHost();
    _tcpsocket->deleteLater();
    _tcpsocket = NULL;
    if(sshChannel) libssh2_channel_free(sshChannel);
#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : Connection" << _name << "closed (" << reason << ")";
#else
    Q_UNUSED(reason);
#endif
    emit disconnected();
}

void SshTunnelOut::sshDataReceived()
{
    char buf[16384];
    ssize_t len = 0,wr = 0;
    int i;

    while (this->sshChannel == NULL)
    {
        this->sshChannel = libssh2_channel_direct_tcpip(this->sshClient->session(), "127.0.0.1", _port);
        if (this->sshChannel == NULL)
        {
            char *errmsg;
            int errlen;
            int err = libssh2_session_last_error(this->sshClient->session(), &errmsg, &errlen, 0);

            if(err == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            qDebug() << "ERROR : SshTunnelOut(" << _name << ") : direct_tcpip failed :" << err << QString::fromLocal8Bit(errmsg, errlen);
            if(this->sshChannel) emit channelReady();
        }
    }

    do
    {
        /* Read data from SSH */
        len = libssh2_channel_read(this->sshChannel, buf, sizeof(buf));
        if (LIBSSH2_ERROR_EAGAIN == len)
        {
            break;
        }
        else if (len < 0)
        {
            qDebug() << "ERROR : " << _name << " remote failed to read (" << len << ")";
            return;
        }

        /* Write data into output local socket */
        wr = 0;
        if(_tcpsocket != NULL)
        {
            while (wr < len)
            {
                i = _tcpsocket->write(buf + wr, len - wr);
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


        if (libssh2_channel_eof(this->sshChannel) && _opened)
        {
            close("channel_eof");
        }
        emit data_rx(len);
    }
    while(len > 0);
}

void SshTunnelOut::tcpDataReceived()
{
    char buf[16384];
    qint64 len = 0;
    ssize_t wr = 0;
    ssize_t i = 0;
    unsigned retry = 100;

    while(!sshChannel && retry--)
    {
        QEventLoop loop;
        QTimer timer;
        timer.start(100);
        connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
        connect(this,SIGNAL(channelReady()),&loop,SLOT(quit()));
        loop.exec();
    }

    if (_tcpsocket == NULL || this->sshChannel == NULL)
    {
        qDebug() << "ERROR : SshTunnelOut(" << _name << ") : received TCP data but not seems to be a valid Tcp socket or channel not ready";
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
            i = libssh2_channel_write(this->sshChannel, buf, len);
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
                wr += i;
            }
        } while(i > 0 && wr < len);
        emit data_tx(len);
    }
    while(len > 0);
}

void SshTunnelOut::tcpDisconnected()
{
    close("socket_diconnected");
}

void SshTunnelOut::displayError(QAbstractSocket::SocketError error)
{
    if(error != QAbstractSocket::RemoteHostClosedError)
    qDebug() << "ERROR : SshTunnelOut(" << _name << ") : redirection socket error=" << error;
}
