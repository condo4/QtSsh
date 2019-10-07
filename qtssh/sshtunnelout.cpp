#include "sshtunnelout.h"
#include "sshclient.h"
#include <QDateTime>

Q_LOGGING_CATEGORY(logsshtunnelout, "ssh.tunnelout", QtWarningMsg)


SshTunnelOut::SshTunnelOut(SshClient *client, const QString &port_identifier, quint16 port):
    SshChannel(port_identifier, client),
    m_sshclient(client),
    m_port(port)
{
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
    QEventLoop wait;
    QObject::connect(this, &SshTunnelOut::connectionClosed, &wait, &QEventLoop::quit);
    for(auto *ch: m_connections)
    {
        ch->disconnectFromHost();
    }

    if(!m_connections.empty())
    {
        qCDebug(logsshtunnelout) << m_name << "Waiting all connection closed";
        wait.exec();
        qCDebug(logsshtunnelout) << m_name << "All connection closed";
    }

    m_tcpserver.close();
    emit closed();
}

void SshTunnelOut::createConnection()
{
    if(!m_sshclient->getSshConnected())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    SshTunnelOutConnection *ch = new SshTunnelOutConnection(m_name, m_sshclient, m_tcpserver, m_port, m_myself.toStrongRef());
    m_connections.push_back(ch);
}

quint16 SshTunnelOut::localPort()
{
    return m_tcpserver.serverPort();
}

void SshTunnelOut::setSharedPointer(QSharedPointer<SshTunnelOut> &ptr)
{
    m_myself = ptr.toWeakRef();
}

void SshTunnelOut::_removeClosedConnection(SshTunnelOutConnection *ch)
{
    m_connections.removeAll(ch);
    ch->deleteLater();
    if(m_connections.empty())
    {
        emit connectionClosed();
    }
}

bool SshTunnelOut::isClosed()
{
    return m_connections.length() == 0;
}
