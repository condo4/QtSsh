#include "sshchannel.h"
#include "sshclient.h"
#include <QCoreApplication>

Q_LOGGING_CATEGORY(sshchannel, "ssh.channel", QtWarningMsg)

SshChannel::SshChannel(QString name, SshClient *client)
    : QObject(client)
    , m_sshClient(client)
    , m_name(name)
{
    qCDebug(sshchannel) << "createChannel:" << m_name;
    m_sshClient->registerChannel(this);
    QObject::connect(m_sshClient, &SshClient::sshDataReceived, this, &SshChannel::sshDataReceived, Qt::QueuedConnection);
}

SshChannel::ChannelState SshChannel::channelState() const
{
    return m_channelState;
}

void SshChannel::setChannelState(const ChannelState &channelState)
{
    if(m_channelState != channelState)
    {
        m_channelState = channelState;
        emit stateChanged(m_channelState);
    }
}

QString SshChannel::name() const
{
    return m_name;
}
