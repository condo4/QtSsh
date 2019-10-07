#include "sshtunnelout.h"
#include "sshclient.h"
#include <QDateTime>

Q_LOGGING_CATEGORY(logsshtunnelout, "ssh.tunnelout", QtWarningMsg)


SshTunnelOut::SshTunnelOut(const QString &name, SshClient *client)
    : SshChannel(name, client)
{
    QObject::connect(&m_tcpserver, &QTcpServer::newConnection, this, &SshTunnelOut::_createConnection);
}

SshTunnelOut::~SshTunnelOut()
{
    qCDebug(logsshtunnelout) << "free Channel:" << m_name;
}

void SshTunnelOut::close()
{
    setChannelState(ChannelState::Close);
    sshDataReceived();
}

void SshTunnelOut::listen(quint16 port)
{
    m_port = port;
    m_tcpserver.listen(QHostAddress("127.0.0.1"), 0);
    setChannelState(ChannelState::Exec);
}

void SshTunnelOut::sshDataReceived()
{
    qCDebug(logsshtunnelout) << "Channel "<< m_name << "State:" << channelState();
    switch(channelState())
    {
        case Openning:
        {
            qCDebug(logsshtunnelout) << "Channel session opened";
            setChannelState(ChannelState::Exec);
        }

        FALLTHROUGH; case Exec:
        {
            setChannelState(ChannelState::Read);
            /* OK, next step */
        }

        FALLTHROUGH; case Read:
        {
            // Nothing to do...
        }

        FALLTHROUGH; case Close:
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


void SshTunnelOut::_createConnection()
{
    if(!m_sshClient->getSshConnected())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    new SshTunnelOutConnection(m_name, m_sshClient, m_tcpserver, m_port);
}

quint16 SshTunnelOut::localPort()
{
    return m_tcpserver.serverPort();
}
