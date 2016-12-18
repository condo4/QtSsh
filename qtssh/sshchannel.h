#ifndef SSHCHANNEL_H
#define SSHCHANNEL_H

#include <QObject>
#include "libssh2.h"

class SshClient;

class SshChannel : public QObject
{
    Q_OBJECT

protected:
    LIBSSH2_CHANNEL *sshChannel;
    SshClient       *sshClient;

public:
    explicit SshChannel(QObject *client);
    explicit SshChannel(SshClient *client);
    virtual ~SshChannel();
    
signals:
    void data_rx(qint64 cnt);
    void data_tx(qint64 cnt);
    
protected slots:
    virtual void sshDataReceived();
    virtual void stopChannel();
};

#endif // SSHCHANNEL_H
