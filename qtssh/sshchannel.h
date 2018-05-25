#ifndef SSHCHANNEL_H
#define SSHCHANNEL_H

#include <QObject>
#include "async_libssh2.h"

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

protected slots:
    virtual void sshDataReceived() {}
    virtual void stopChannel();

public slots:
    virtual quint16 localPort() { return 0; }
};

#endif // SSHCHANNEL_H
