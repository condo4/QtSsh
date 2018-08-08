#pragma once

#include "sshchannel.h"

class SshScpGet : public SshChannel
{
    Q_OBJECT

public:
    SshScpGet(SshClient * client);
    QString get(QString source, QString dest);
};
