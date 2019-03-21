#pragma once

#include <QObject>
#include <QList>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include "sshchannel.h"
#include "sshkey.h"

Q_DECLARE_LOGGING_CATEGORY(sshclient)

class  SshClient : public QObject {
    Q_OBJECT

private:
    LIBSSH2_SESSION    * m_session;
    LIBSSH2_KNOWNHOSTS * m_knownHosts;
    static int s_nbInstance;
    QString m_name;
    QMap<QString, QSharedPointer<SshChannel>> m_channels;
    QTcpSocket m_socket;

    quint16 m_port;
    int m_errorcode;
    bool m_sshConnected;

    QString m_hostname;
    QString m_username;
    QString m_passphrase;
    QString m_privateKey;
    QString m_publicKey;
    QString m_errorMessage;
    QString m_knowhostFiles;
    SshKey  m_hostKey;
    QTimer m_keepalive;
    QMutex channelCreationInProgress;
    void *currentLockerForChannelCreation;

public:
    SshClient(const QString &name = "noname", QObject * parent = nullptr);
    virtual ~SshClient();

    QString getName() const;
    bool takeChannelCreationMutex(void *identifier);
    void releaseChannelCreationMutex(void *identifier);

public slots:
    int connectToHost(const QString & username, const QString & hostname, quint16 port = 22);
    void disconnectFromHost();
    QString runCommand(const QString &command);
    quint16 openLocalPortForwarding(const QString &servicename, quint16 port, quint16 bind);
    quint16 openRemotePortForwarding(const QString &servicename, quint16 port);
    void closePortForwarding(const QString &servicename);
    void setKeys(const QString &publicKey, const QString &privateKey);
    QString sendFile(const QString &src, const QString &dst);
    void setPassphrase(const QString & pass);
    bool saveKnownHosts(const QString &file);
    void setKownHostFile(const QString &file);
    bool addKnownHost  (const QString &hostname, const SshKey &key);
    QString banner();
    void waitSocket();

public slots:

    LIBSSH2_SESSION *session();
    bool channelReady();
    bool loopWhileBytesWritten(int msecs);
    bool getSshConnected() const;


signals:
    void sshDataReceived();
    void _connectionFailed();

    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError);
    void stateChanged(QAbstractSocket::SocketState socketState);

private slots:
    void _readyRead();
    void _disconnected();
    void _getLastError();
    void _sendKeepAlive();
    bool _loadKnownHosts(const QString &file);

    void _tcperror(QAbstractSocket::SocketError err);
    void _stateChanged(QAbstractSocket::SocketState socketState);
};
