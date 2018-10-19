#pragma once

#include "sshchannel.h"

class SshScpGet : public SshChannel
{
    Q_OBJECT

public:
    SshScpGet(SshClient * client);
    QString get(const QString &source, const QString &dest);
};
