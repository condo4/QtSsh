#pragma once

#include "sshchannel.h"

class SshScpGet : public SshChannel
{
    Q_OBJECT

protected:
    SshScpGet(const QString &name, SshClient *client);
    friend class SshClient;

public:
    QString get(const QString &source, const QString &dest);
};
