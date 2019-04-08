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
}

SshChannel::~SshChannel()
{
    free();
}

bool SshChannel::connected() const
{
    return m_connected;
}

QString SshChannel::name() const
{
    return m_name;
}

void SshChannel::connectChannel(LIBSSH2_CHANNEL *channel)
{
    m_sshChannel = channel;
    m_connected = true;
    qCDebug(sshchannel) << "connectChannel:" << m_name;
}

void SshChannel::disconnectChannel()
{
    free();
}

void SshChannel::close()
{
    if (m_sshChannel == nullptr || m_connected == false || !m_sshClient->getSshConnected())
        return;

    qCDebug(sshchannel) << "closeChannel:" << m_name;
    int ret = qssh2_channel_close(m_sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
        return;
    }

    ret = qssh2_channel_wait_closed(m_sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_wait_closed";
        return;
    }
    m_connected = false;
}

void SshChannel::free()
{
    if (m_sshChannel == nullptr)
        return;

    close();

    qCDebug(sshchannel) << "freeChannel:" << m_name;

    int ret = qssh2_channel_free(m_sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_free";
        return;
    }
    m_sshChannel = nullptr;
}
