#include "sshtunneloutsrv.h"
#include "sshtunnelout.h"
#include "sshclient.h"

SshTunnelOutSrv::SshTunnelOutSrv(SshClient *client, QString port_identifier, quint16 port):
    SshChannel(client),
    m_sshclient(client),
    m_identifier(port_identifier),
    m_port(port),
    m_count(0)
{
    m_tcpserver.listen(QHostAddress("127.0.0.1"), 0);
    QObject::connect(&m_tcpserver, &QTcpServer::newConnection, this, &SshTunnelOutSrv::createConnection);
}

SshTunnelOutSrv::~SshTunnelOutSrv()
{
    foreach (SshTunnelOut *tunnel, m_connections) {
        QObject::disconnect(sshClient, &SshClient::sshDataReceived, tunnel, &SshTunnelOut::sshDataReceived);
        QObject::disconnect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
        delete tunnel;
    }
    m_tcpserver.close();
    m_tcpserver.deleteLater();
}

void SshTunnelOutSrv::createConnection()
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::createConnection()";
#endif

    if(!m_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    QTcpSocket *sock = m_tcpserver.nextPendingConnection();
    if(!sock) return;

    SshTunnelOut *tunnel = new SshTunnelOut(m_sshclient, sock, QString("%1_%2").arg(m_identifier).arg(++m_count), m_port, this);
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
    m_connections.append(tunnel);
}

void SshTunnelOutSrv::connectionDisconnected()
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::connectionDisconnected()";
#endif
    SshTunnelOut *tunnel = qobject_cast<SshTunnelOut *>(QObject::sender());
    if(tunnel == nullptr)
        return;
    m_connections.removeAll(tunnel);
    QObject::disconnect(tunnel,    &SshTunnelOut::disconnected, this,   &SshTunnelOutSrv::connectionDisconnected);
    delete tunnel;

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshTunnelOut : SshTunnelOutSrv::connectionDisconnected() OK";
#endif

}


quint16 SshTunnelOutSrv::localPort()
{
    return m_tcpserver.serverPort();
}
