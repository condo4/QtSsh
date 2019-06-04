#include "sshtunnelout.h"
#include "sshclient.h"
#include <QDateTime>

Q_LOGGING_CATEGORY(logsshtunnelout, "ssh.tunnelout", QtWarningMsg)


SshTunnelOut::SshTunnelOut(SshClient *client, const QString &port_identifier, quint16 port):
    SshChannel(port_identifier, client),
    m_sshclient(client),
    m_port(port)
{
    unmanaged();
    m_tcpserver.listen(QHostAddress("127.0.0.1"), 0);
    QObject::connect(&m_tcpserver, &QTcpServer::newConnection, this, &SshTunnelOut::createConnection);
}

SshTunnelOut::~SshTunnelOut()
{
    qCDebug(logsshtunnelout) << m_name << "Destruction";
    close();
    qCDebug(logsshtunnelout) << m_name << "Destruction completed";
}

void SshTunnelOut::close()
{
    qCDebug(logsshtunnelout) << m_name << "Close server";
    m_tcpserver.close();

    if(m_connections.size() != 0)
    {
        QEventLoop wait;
        QObject::connect(this, &SshTunnelOut::closed, &wait, &QEventLoop::quit);

        for(auto *ch: m_connections)
        {
            ch->disconnectFromHost();
        }

        wait.exec();
    }
}

void SshTunnelOut::createConnection()
{
    if(!m_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    QTcpSocket *sock = m_tcpserver.nextPendingConnection();

    connect(sock, &QTcpSocket::disconnected, [sock](){qCDebug(logsshtunnelout) << "SOCK DISCONNECTED: " << sock << sock->localPort();});
    if(!sock) return;
    qCDebug(logsshtunnelout) << m_name << "createConnection: " << sock << sock->localPort();

    SshTunnelOutConnection *ch = new SshTunnelOutConnection(m_name, m_sshclient, sock, m_port, this);
    m_connections.push_back(ch);
}

quint16 SshTunnelOut::localPort()
{
    return m_tcpserver.serverPort();
}

void SshTunnelOut::_removeClosedConnection(SshTunnelOutConnection *ch)
{
    m_connections.removeAll(ch);
    ch->deleteLater();
    if(m_connections.size() == 0)
    {
        emit closed();
    }
}

bool SshTunnelOut::isClosed()
{
    return m_connections.length() == 0;
}
