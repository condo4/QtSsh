#include "sshchannel.h"
#include "sshclient.h"

SshChannel::SshChannel(QObject *client) : QObject(client)
{

}

SshChannel::SshChannel(SshClient *client) :
    QObject(client),
    sshChannel(NULL),
    sshClient(client)
{
    connect(sshClient, SIGNAL(sshDataReceived()), this, SLOT(sshDataReceived()));
    connect(sshClient, SIGNAL(sshReset()), this, SLOT(stopChannel()));
    connect(this, SIGNAL(data_tx(qint64)), sshClient, SLOT(tx_data(qint64)));
    connect(this, SIGNAL(data_rx(qint64)), sshClient, SLOT(rx_data(qint64)));
}

SshChannel::~SshChannel()
{
}

void SshChannel::sshDataReceived()
{
}

void SshChannel::stopChannel()
{
    if (sshChannel != NULL)
    {
        libssh2_channel_close(sshChannel);
        libssh2_channel_wait_closed(sshChannel);
        libssh2_channel_free(sshChannel);
        sshChannel = NULL;
    }
}
