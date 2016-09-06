#ifndef SSHWORKER_H
#define SSHWORKER_H

#include <QObject>
#include <QThread>
#include "sshclient.h"

class SshWorker : public QThread
{
    Q_OBJECT
    SshClient *_client;

private slots:
    void xferRate(qint64 tx, qint64 rx);

public:
    explicit SshWorker(QObject *parent = 0);
    ~SshWorker();

    void setKeys(const QString &publicKey, const QString &privateKey);
    bool loadKnownHosts(const QString &file);
    int connectSshToHost(const QString & username, const QString & hostname, quint16 port = 22, bool lock = true, bool checkHostKey = false, unsigned int retry = 5);
    void disconnectSshFromHost();
    QString runCommand(QString command);
    quint16 openLocalPortForwarding(QString servicename, quint16 port, quint16 bind);
    quint16 openRemotePortForwarding(QString servicename, quint16 port);
    void closePortForwarding(QString servicename);
    QString sendFile(QString src, QString dst);

    /* SFTP Methode wrapper */
    void enableSftp();
    QString sFtpSend(QString source, QString dest);
    bool sFtpGet(QString source, QString dest, bool override = false);
    int sFtpMkdir(QString dest);
    QStringList sFtpDir(QString d);
    bool sFtpIsDir(QString d);
    bool sFtpIsFile(QString d);
    int sFtpMkpath(QString dest);
    bool sFtpUnlink(QString d);

signals:
    void threadReady();
    void xfer_rate(qint64 tx, qint64 rx);
    void askQuit();
    void askSetKeys(const QString &publicKey, const QString &privateKey);
    void askLoadKnownHosts(const QString &file);
    void askConnectSshToHost(const QString & username, const QString & hostname, quint16 port = 22, bool lock = true, bool checkHostKey = false, unsigned int retry = 5);
    void askDisconnectSshFromHost();
    void askRunCommand(QString command);
    void askOpenLocalPortForwarding(QString servicename, quint16 port, quint16 bind);
    void askOpenRemotePortForwarding(QString servicename, quint16 port);
    void askClosePortForwarding(QString servicename);
    void askSendFile(QString src, QString dst);

    void askEnableSftp();
    QString askSFtpSend(QString source, QString dest);
    bool askSFtpGet(QString source, QString dest, bool override = false);
    int askSFtpMkdir(QString dest);
    QStringList askSFtpDir(QString d);
    bool askSFtpIsDir(QString d);
    bool askSFtpIsFile(QString d);
    int askSFtpMkpath(QString dest);
    bool askSFtpUnlink(QString d);
    void sFtpXfer();


public slots:
    void run();


};

#endif // SSHWORKER_H
