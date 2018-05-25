#include "sshchannel.h"
#include "sshclient.h"
#include <QCoreApplication>

SshChannel::SshChannel(QObject *client) : QObject(client)
{

}

SshChannel::SshChannel(SshClient *client) :
    QObject(client),
    sshChannel(nullptr),
    sshClient(client)
{
    connect(sshClient, SIGNAL(sshReset()), this, SLOT(stopChannel()));
}

SshChannel::~SshChannel()
{
    stopChannel();
}

void SshChannel::stopChannel()
{
    if (sshChannel == nullptr)
        return;

    int ret = qssh2_channel_close(sshChannel);

    if(ret)
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshChannel() : Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
#endif
        return;
    }

    ret = qssh2_channel_wait_closed(sshChannel);
    if(ret)
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshChannel() : Failed to channel_wait_closed";
#endif
        return;
    }

    ret = qssh2_channel_free(sshChannel);
    if(ret)
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshChannel() : Failed to channel_free";
#endif
        return;
    }
    sshChannel = nullptr;
}
