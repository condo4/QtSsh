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

QString SshChannel::name() const
{
    return m_name;
}
