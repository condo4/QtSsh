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
        tunnel->deleteLater();
    }
    _tcpserver.close();
    _tcpserver.deleteLater();
}

void SshTunnelOutSrv::createConnection()
{
    if(!_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    QTcpSocket *sock = _tcpserver.nextPendingConnection();
    if(!sock) return;

    SshTunnelOut *tunnel = new SshTunnelOut(_sshclient, sock, QString("%1_%2").arg(_identifier).arg(++_count), _port);
    if(!tunnel->ready())
    {
        sock->close();
        delete tunnel;
        return;
    }
    QObject::connect(sshClient, &SshClient::sshDataReceived, tunnel, &SshTunnelOut::sshDataReceived, Qt::QueuedConnection);
    QObject::connect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
    _connections.append(tunnel);
}

void SshTunnelOutSrv::connectionDisconnected()
{
    SshTunnelOut *tunnel = qobject_cast<SshTunnelOut *>(QObject::sender());
    if(tunnel == nullptr)
        return;
    _connections.removeAll(tunnel);
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, tunnel, &SshTunnelOut::sshDataReceived);
    QObject::disconnect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
    tunnel->deleteLater();
}

quint16 SshTunnelOutSrv::localPort()
{
    return _tcpserver.serverPort();
}
