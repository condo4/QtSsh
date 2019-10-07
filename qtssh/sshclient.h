#pragma once

#include <QObject>
#include <QList>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include "sshchannel.h"
#include "sshkey.h"
#include <QSharedPointer>

#ifndef FALLTHROUGH
#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]]
#else
#define FALLTHROUGH
#endif
#endif

Q_DECLARE_LOGGING_CATEGORY(sshclient)
class SshProcess;
class SshScpGet;
class SshScpSend;
class SshSFtp;
class SshTunnelIn;
class SshTunnelOut;

class  SshClient : public QObject {
    Q_OBJECT

public:
    void unregisterChannel(SshChannel* channel);
    void registerChannel(SshChannel* channel);

private:
    static int s_nbInstance;
    LIBSSH2_SESSION    * m_session {nullptr};
    LIBSSH2_KNOWNHOSTS * m_knownHosts {nullptr};
    QList<SshChannel*> m_channels;
    QAbstractSocket::SocketState m_state {QAbstractSocket::UnconnectedState};



    QString m_name;
    QTcpSocket m_socket;
    qint64 m_lastProofOfLive {0};

    quint16 m_port {0};
    int m_errorcode {0};
    bool m_sshConnected {false};

    QString m_hostname;
    QString m_username;
    QString m_passphrase;
    QString m_privateKey;
    QString m_publicKey;
    QString m_errorMessage;
    QString m_knowhostFiles;
    SshKey  m_hostKey;
    QTimer m_keepalive;
    QTimer m_connectionTimeout;
    QMutex channelCreationInProgress;
    void *currentLockerForChannelCreation {nullptr};

    void _sshClientClose();
    void _sshClientFree();

public:
    SshClient(const QString &name = "noname", QObject * parent = nullptr);
    virtual ~SshClient();

    QString getName() const;
    bool takeChannelCreationMutex(void *identifier);
    void releaseChannelCreationMutex(void *identifier);


    void setState(const QAbstractSocket::SocketState &state);

public slots:
    int connectToHost(const QString & username, const QString & hostname, quint16 port = 22, QByteArrayList methodes = QByteArrayList());
    void disconnectFromHost();

    SshProcess *getSshProcess(const QString &name = "cmd");
    SshScpSend *getSshScpSend(const QString &name = "scp_send");
    SshScpGet *getSshScpGet(const QString &name = "scp_get");
    SshSFtp *getSFtp(const QString &name = "sftp");
    SshTunnelIn* getTunnelIn(const QString &name, quint16 localport, quint16 remoteport = 0, QString host = "127.0.0.1");
    SshTunnelOut *getTunnelOut(const QString &name, quint16 port);

    void setKeys(const QString &publicKey, const QString &privateKey);
    void setPassphrase(const QString & pass);
    bool saveKnownHosts(const QString &file);
    void setKownHostFile(const QString &file);
    bool addKnownHost  (const QString &hostname, const SshKey &key);
    QString banner();
    void waitSocket();


    LIBSSH2_SESSION *session();
    bool loopWhileBytesWritten(int msecs);
    bool getSshConnected() const;


signals:
    void sshDataReceived();
    void sshDisconnected();

    void _connectionFailed();

    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError);
    void stateChanged(QAbstractSocket::SocketState socketState);

private slots:
    void _disconnected();
    void _sendKeepAlive();





public: /* New function implementation with state machine */
    enum SshState {
        Unconnected,
        SocketConnection,
        WaitingSocketConnection,
        Initialize,
        HandShake,
        GetAuthenticationMethodes,
        Authentication,
        Ready,
        DisconnectChannel,
        Error
    };
    Q_ENUM(SshState)
    SshState sshState() const;

private: /* New function implementation with state machine */
    SshState m_sshState {SshState::Unconnected};
    QByteArrayList m_authenticationMethodes;
    void setSshState(const SshState &sshState);


private slots: /* New function implementation with state machine */
    void _connection_socketTimeout();
    void _connection_socketError();
    void _connection_socketConnected();
    void _connection_socketDisconnected();
    void _ssh_processEvent();

signals:
    void sshStateChanged(SshState sshState);
    void sshReady();
    void sshError();
};
