#include "sshtunnelout.h"
#include "sshclient.h"
#include <QDateTime>

Q_LOGGING_CATEGORY(logsshtunnelout, "ssh.tunnelout", QtWarningMsg)


SshTunnelOut::SshTunnelOut(const QString &name, SshClient *client)
    : SshChannel(name, client)
{
    QObject::connect(&m_tcpserver, &QTcpServer::newConnection, this, &SshTunnelOut::_createConnection);
    sshDataReceived();
}

SshTunnelOut::~SshTunnelOut()
{
    qCDebug(logsshtunnelout) << "free Channel:" << m_name;
}

void SshTunnelOut::close()
{
    qCDebug(logsshtunnelout) << m_name << "Ask to close";
    setChannelState(ChannelState::Close);
    sshDataReceived();
}

void SshTunnelOut::listen(quint16 port)
{
    m_port = port;
    m_tcpserver.listen(QHostAddress("127.0.0.1"), 0);
    setChannelState(ChannelState::Read);
}

void SshTunnelOut::sshDataReceived()
{
    switch(channelState())
    {
        case Openning:
        {
            qCDebug(logsshtunnelout) << "Channel session opened";
            setChannelState(ChannelState::Exec);
        }

        FALLTHROUGH; case Exec:
        {
            /* OK, next step */
        }

        case Read:
        {
            // Nothing to do...
            return;
        }

        case Close:
        {
            qCDebug(logsshtunnelout) << m_name << "Close server";
            m_tcpserver.close();
            setChannelState(ChannelState::WaitClose);
        }

        FALLTHROUGH; case WaitClose:
        {
            qCDebug(logsshtunnelout) << "Wait close channel:" << m_name;
            setChannelState(ChannelState::Freeing);
        }

        FALLTHROUGH; case Freeing:
        {
            qCDebug(logsshtunnelout) << "free Channel:" << m_name;
            setChannelState(ChannelState::Free);

            QObject::disconnect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelOut::sshDataReceived);
            emit canBeDestroy(this);
            return;
        }

        case Free:
        {
            qCDebug(logsshtunnelout) << "Channel" << m_name << "is free";
            return;
        }

        case Error:
        {
            qCDebug(logsshtunnelout) << "Channel" << m_name << "is in error state";
            return;
        }
    }
}

quint16 SshTunnelOut::port() const
{
    return m_port;
}


void SshTunnelOut::_createConnection()
{
    qCDebug(logsshtunnelout) << "SshTunnelOut new connection";
    SshTunnelOutConnection *connection = new SshTunnelOutConnection(m_name + QString("_%1").arg(m_connectionCounter++), m_sshClient, m_tcpserver, m_port);
    QObject::connect(connection, &SshTunnelOutConnection::canBeDestroy, this, &SshTunnelOut::_destroyConnection);
    m_connection.append(connection);
    emit connectionChanged(m_connection.count());
}

void SshTunnelOut::_destroyConnection(SshChannel *ch)
{
    SshTunnelOutConnection *connection = qobject_cast<SshTunnelOutConnection*>(ch);
    if(connection)
    {
        m_connection.removeAll(connection);
        emit connectionChanged(m_connection.count());
    }
}

quint16 SshTunnelOut::localPort()
{
    return m_tcpserver.serverPort();
}
