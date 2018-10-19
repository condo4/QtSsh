#pragma once

#include "sshchannel.h"

class SshScpSend : public SshChannel
{
    Q_OBJECT

public:
    SshScpSend(SshClient * client);
    QString send(const QString &source, QString dest);
};
