#ifndef SSHPROCESS_H
#define SSHPROCESS_H
#include "sshchannel.h"
#include <QSemaphore>

class SshProcess : public SshChannel
{
    Q_OBJECT

public:
    explicit SshProcess(SshClient *client);
    virtual ~SshProcess();
    
public slots:
    QByteArray runCommand(QString cmd);

};

#endif // SSHPROCESS_H
