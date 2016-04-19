#ifndef SSHPROCESS_H
#define SSHPROCESS_H
#include "sshchannel.h"
#include <QSemaphore>

class SshProcess : public SshChannel
{
    Q_OBJECT

    enum TerminalType{
        VanillaTerminal,
        Vt102Terminal,
        AnsiTerminal
    };

    enum SshProcessState {
        NoState,
        OpenChannelSession,
        EarlyCmdTransition,
        StartCmdProcess,
        RequestPty,
        ErrorNoRetry,
        ReadyRead
    };

    SshProcessState _currentState;
    QSemaphore _channelInUse;
    bool _isCommand;
    bool _isEOF;
    QString _cmd;
    QList<SshProcessState> _nextActions;

public:
    explicit SshProcess(SshClient *client);
    virtual ~SshProcess();
    QString result();
    qint64 readData(char * buff, qint64 len);

signals:
    void readyRead();
    void connected();
    
public slots:
    void start(QString cmd);

protected slots:
    virtual void sshDataReceived();
    
};

#endif // SSHPROCESS_H
