#pragma once

#include "sshchannel.h"

class SshScpSend : public SshChannel
{
    Q_OBJECT

protected:
    SshScpSend(const QString &name, SshClient * client);
    friend class SshClient;

public:
    QString send(const QString &source, QString dest);
};
