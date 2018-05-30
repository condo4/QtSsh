#include "sshtunneloutsrv.h"
#include "sshtunnelout.h"
#include "sshclient.h"

SshTunnelOutSrv::SshTunnelOutSrv(SshClient *client, QString port_identifier, quint16 port):
    SshChannel(client),
    _sshclient(client),
    _identifier(port_identifier),
    _port(port),
    _count(0)
{
    _tcpserver.listen(QHostAddress("127.0.0.1"), 0);
    QObject::connect(&_tcpserver, &QTcpServer::newConnection, this, &SshTunnelOutSrv::createConnection);
}

SshTunnelOutSrv::~SshTunnelOutSrv()
{
    foreach (SshTunnelOut *tunnel, _connections) {
        QObject::disconnect(sshClient, &SshClient::sshDataReceived, tunnel, &SshTunnelOut::sshDataReceived);
        QObject::disconnect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
        delete tunnel;
    }
    _tcpserver.close();
    _tcpserver.deleteLater();
}

void SshTunnelOutSrv::createConnection()
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::createConnection()";
#endif

    if(!_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    QTcpSocket *sock = _tcpserver.nextPendingConnection();
    if(!sock) return;

    SshTunnelOut *tunnel = new SshTunnelOut(_sshclient, sock, QString("%1_%2").arg(_identifier).arg(++_count), _port, this);
    if(!tunnel->ready())
    {
        sock->close();
        tunnel->deleteLater();
        return;
    }
    QObject::connect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::createConnection() OK";
#endif
    _connections.append(tunnel);
}

void SshTunnelOutSrv::connectionDisconnected()
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::connectionDisconnected()";
#endif
    SshTunnelOut *tunnel = qobject_cast<SshTunnelOut *>(QObject::sender());
    if(tunnel == nullptr)
        return;
    _connections.removeAll(tunnel);
    QObject::disconnect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
    delete tunnel;

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::connectionDisconnected() OK";
#endif

}


quint16 SshTunnelOutSrv::localPort()
{
    return _tcpserver.serverPort();
}
