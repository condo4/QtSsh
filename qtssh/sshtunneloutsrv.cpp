#include "sshtunneloutsrv.h"
#include "sshtunnelout.h"
#include "sshclient.h"

SshTunnelOutSrv::SshTunnelOutSrv(SshClient *client, QString port_identifier, quint16 port):
    QObject(client),
    _sshclient(client),
    _identifier(port_identifier),
    _port(port)
{
    _tcpserver = new QTcpServer(this);
    _tcpserver->listen(QHostAddress("127.0.0.1"), 0);
    connect(_tcpserver, SIGNAL(newConnection()), this, SLOT  (createConnection()));
}

SshTunnelOutSrv::~SshTunnelOutSrv()
{
    _tcpserver->close();
    _tcpserver->deleteLater();
}

void SshTunnelOutSrv::createConnection()
{
    static int count = 0;

    if(!_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    SshTunnelOut *tunnel = new SshTunnelOut(_sshclient, _tcpserver->nextPendingConnection(), QString("%1_%2").arg(_identifier).arg(++count), _port);
    connect(tunnel,SIGNAL(disconnected()), this, SLOT(connectionDisconnected()));
    _connections.append(tunnel);
}

void SshTunnelOutSrv::connectionDisconnected()
{
    SshTunnelOut *tunnel = (SshTunnelOut *)QObject::sender();
    if(tunnel == NULL)
        return;
    _connections.removeAll(tunnel);
    tunnel->deleteLater();
}

quint16 SshTunnelOutSrv::localPort()
{
    return _tcpserver->serverPort();
}
