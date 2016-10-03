#ifndef SSHSCPSEND_H
#define SSHSCPSEND_H


#include "sshchannel.h"

enum SshScpSendState {
    ScpError = 0,
    ScpPrepare = 1,
    ScpCopy = 2,
    ScpEof = 3,
    ScpClose = 4,
    ScpEnd = 5
};

class SshScpSend : public SshChannel
{
    Q_OBJECT

    QString _source;
    QString _destination;
    SshScpSendState _currentState;
    FILE *_local;
    struct stat _fileinfo;
    bool _state;

protected slots:
    void sshDataReceived();

public:
    SshScpSend(SshClient * client, QString source, QString dest);
    QString send();

    bool state() const;

signals:
    void transfertTerminate();
};

#endif // SSHSCPSEND_H
