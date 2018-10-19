#include "sshchannel.h"
#include "sshclient.h"
#include <QCoreApplication>

Q_LOGGING_CATEGORY(sshchannel, "ssh.channel", QtWarningMsg)

SshChannel::SshChannel(QObject *client):
    QObject(client),
    sshChannel(nullptr),
    sshClient(nullptr)
{

}

SshChannel::SshChannel(SshClient *client) :
    QObject(client),
    sshChannel(nullptr),
    sshClient(client)
{
    connect(sshClient, &SshClient::sshReset, this, &SshChannel::stopChannel);
}

SshChannel::~SshChannel()
{
    disconnect(sshClient, &SshClient::sshReset, this, &SshChannel::stopChannel);
    stopChannel();
}

void SshChannel::stopChannel()
{
    if (sshChannel == nullptr)
        return;

    int ret = qssh2_channel_close(sshChannel);

    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
        return;
    }

    ret = qssh2_channel_wait_closed(sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_wait_closed";
        return;
    }

    ret = qssh2_channel_free(sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_free";
        return;
    }
    sshChannel = nullptr;
}
