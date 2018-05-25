#ifndef SSHSCPSEND_H
#define SSHSCPSEND_H


#include "sshchannel.h"

class SshScpSend : public SshChannel
{
    Q_OBJECT

public:
    SshScpSend(SshClient * client);
    QString send(QString source, QString dest);
};

#endif // SSHSCPSEND_H
