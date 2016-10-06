#ifndef SSHINTERFACE_H
#define SSHINTERFACE_H

class SshKey {
    public:
        enum Type {
            UnknownType,
            Rsa,
            Dss
        };
        QByteArray hash;
        QByteArray key;
        Type       type;
};

class SshInterface
{
public slots:
    virtual ~SshInterface() {}
    virtual void disconnectFromHost() = 0;
    virtual int connectToHost(const QString & username, const QString & hostname, quint16 port = 22, bool lock = true, bool checkHostKey = false, unsigned int retry = 5) = 0;
    virtual QString runCommand(QString command) = 0;
    virtual quint16 openLocalPortForwarding(QString servicename, quint16 port, quint16 bind) = 0;
    virtual quint16 openRemotePortForwarding(QString servicename, quint16 port) = 0;
    virtual void closePortForwarding(QString servicename) = 0;
    virtual void setKeys(const QString &publicKey, const QString &privateKey) = 0;
    virtual bool loadKnownHosts(const QString &file) = 0;
    virtual QString sendFile(QString src, QString dst) = 0;
    virtual void setPassphrase(const QString & pass) = 0;
    virtual bool saveKnownHosts(const QString &file) = 0;
    virtual bool addKnownHost  (const QString &hostname, const SshKey &key) = 0;
};

#endif // SSHINTERFACE_H
