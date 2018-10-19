#pragma once

#include "sshchannel.h"
#include <QSemaphore>

class SshProcess : public SshChannel
{
    Q_OBJECT

public:
    explicit SshProcess(SshClient *client);
    virtual ~SshProcess();
    
public slots:
    QByteArray runCommand(const QString &cmd);

};
