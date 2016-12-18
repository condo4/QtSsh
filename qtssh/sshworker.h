#ifndef SSHWORKER_H
#define SSHWORKER_H

#include <QObject>
#include <QThread>
#include "sshfsinterface.h"
#include "sshinterface.h"
#include "sshclient.h"

class SshWorker : public QThread, public SshFsInterface, public SshInterface
{
    Q_OBJECT
    SshClient *_client;

public:
    explicit SshWorker(QObject *parent = 0);
    virtual ~SshWorker();

/* <<<SshInterface>>> */
public slots:
    int connectToHost(const QString & username, const QString & hostname, quint16 port = 22, bool lock = true, bool checkHostKey = false, unsigned int retry = 5);
    void disconnectFromHost();
    QString runCommand(QString command);
    quint16 openLocalPortForwarding(QString servicename, quint16 port, quint16 bind);
    quint16 openRemotePortForwarding(QString servicename, quint16 port);
    void closePortForwarding(QString servicename);
    void setKeys(const QString &publicKey, const QString &privateKey);
    bool loadKnownHosts(const QString &file);
    QString sendFile(QString src, QString dst);
    void setPassphrase(const QString & pass);
    bool saveKnownHosts(const QString &file);
    bool addKnownHost  (const QString &hostname, const SshKey &key);
/* >>>SshInterface<<< */


/* <<<SshFsInterface>>> */
public slots:
    void enableSFTP();
    QString send(QString source, QString dest);
    bool get(QString source, QString dest, bool override = false);
    int mkdir(QString dest);
    QStringList readdir(QString d);
    bool isDir(QString d);
    bool isFile(QString d);
    int mkpath(QString dest);
    bool unlink(QString d);
    quint64 filesize(QString d);
/* >>>SshFsInterface<<< */

private slots:
    void xferRate(qint64 tx, qint64 rx);

public slots:
    void run();

signals:
    void threadReady();
    void xfer_rate(qint64 tx, qint64 rx);
    void askQuit();


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
};

#endif // SSHWORKER_H
