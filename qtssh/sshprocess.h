#pragma once

#include "sshchannel.h"
#include <QSemaphore>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logsshprocess)

class SshProcess : public SshChannel
{
    Q_OBJECT

protected:
    explicit SshProcess(const QString &name, SshClient *client);
    friend class SshClient;
    
public slots:
    QByteArray runCommand(const QString &cmd);

};
