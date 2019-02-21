#pragma once

#include "sshchannel.h"
#include <QSemaphore>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logsshprocess)

class SshProcess : public SshChannel
{
    Q_OBJECT

public:
    explicit SshProcess(SshClient *client);
    virtual ~SshProcess();
    
public slots:
    QByteArray runCommand(const QString &cmd);

};
