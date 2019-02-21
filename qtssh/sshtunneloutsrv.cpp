#include "sshtunneloutsrv.h"
#include "sshtunnelout.h"
#include "sshclient.h"

Q_LOGGING_CATEGORY(logsshtunneloutsrv, "ssh.tunneloutsrv", QtWarningMsg)

SshTunnelOutSrv::SshTunnelOutSrv(SshClient *client, const QString &port_identifier, quint16 port):
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
        tunnel->disconectFromHost();
    }
    m_tcpserver.close();
}

void SshTunnelOutSrv::createConnection()
{
    qCDebug(logsshtunneloutsrv) << "SshTunnelOutSrv::createConnection()";

    if(!m_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    QTcpSocket *sock = m_tcpserver.nextPendingConnection();
    if(!sock) return;

    SshTunnelOut *tunnel = new SshTunnelOut(m_sshclient, sock, QString("%1_%2").arg(m_identifier).arg(++m_count), m_port, this);
    QObject::connect(tunnel,    &SshTunnelOut::destroyed,    this,   &SshTunnelOutSrv::connectionDestroyed);
    qCDebug(logsshtunneloutsrv) << "SshTunnelOutSrv::createConnection() OK";
    m_connections.append(tunnel);
}

void SshTunnelOutSrv::connectionDestroyed()
{
    SshTunnelOut *tunnel = qobject_cast<SshTunnelOut *>(QObject::sender());
    if(tunnel == nullptr)
        return;
    m_connections.removeAll(tunnel);
}

quint16 SshTunnelOutSrv::localPort()
{
    return m_tcpserver.serverPort();
}
