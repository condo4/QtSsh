#ifndef QT_SSH_CLIENT_H
#define QT_SSH_CLIENT_H

#include <QObject>
#include <QList>
#include <QTcpSocket>
#include <QTimer>
#include "sshserviceport.h"
#include "debug.h"

extern "C" {
#include <libssh2.h>
#include <errno.h>
}

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

class  SshClient : public QObject
{
    Q_OBJECT

public:
    enum Error {
        NoError = 0,
        AuthenticationError,
        HostKeyUnknownError,
        HostKeyInvalidError,
        HostKeyMismatchError,
        ConnectionRefusedError,
        UnexpectedShutdownError,
        HostNotFoundError,
        SocketError,
        TimeOut,
        UnknownError
    };

private:

    enum SshState {
        NoState = 0,
        TcpHostConnected = 1,
        InitializeSession = 2,
        RequestAuthTypes = 3,
        LookingAuthOptions = 4,
        TryingAuthentication = 5,
        ActivatingChannels = 6
    };

    enum AuthenticationMethod {
        PasswordAuthentication,
        PublicKeyAuthentication
    };


    // libssh2 stuff
    LIBSSH2_SESSION    * _session;
    LIBSSH2_KNOWNHOSTS * _knownHosts;
    QMap<QString,SshServicePort*>   _channels;
    QTcpSocket _socket;

    SshState _state;

    int _port;
    int _errorcode;
    bool _sshConnected;

    QString _hostname;
    QString _username;
    QString _passphrase;
    QString _privateKey;
    QString _publicKey;

    QString _errorMessage;

    SshKey  _hostKey;

    SshClient::Error _delayError;

    // authentication methods
    SshClient::AuthenticationMethod        _currentAuthTry;
    QList<SshClient::AuthenticationMethod> _availableMethods;
    QList<SshClient::AuthenticationMethod> _failedMethods;

    qint64 _cntTxData;
    qint64 _cntRxData;
    QTimer _cntTimer;
    QTimer _keepalive;

public:
    SshClient(QObject * parent = NULL);
    ~SshClient();

    void disconnectFromHost();
    void setPassphrase(const QString & pass);


    bool saveKnownHosts(const QString &file) const;
    bool addKnownHost  (const QString &hostname, const SshKey &key);

    quint16 sshSocketLocalPort();

    QString hostName() const;
    SshKey hostKey() const;

    LIBSSH2_SESSION *session();

    bool channelReady();

    bool waitForBytesWritten(int msecs);


signals:
    void sshConnected();
    void sshDisconnected();
    void sshError(SshClient::Error error);
    void sshAuthenticationRequired(QList<SshClient::AuthenticationMethod> availableMethods);
    void xfer_rate(qint64 tx, qint64 rx);
    void sshDataReceived();
    void sshReset();
    void _connectionTerminate();
    void portForwardingOpened(QString name);
    void portForwardingClosed(QString name);

    void setKeysTerminate();
    void loadKnownHostsTerminate(bool res);
    void connectSshToHostTerminate(int res);
    void runCommandTerminate(QString res);
    void disconnectSshFromHostTerminate();
    void openLocalPortForwardingTerminate(quint16);
    void openRemotePortForwardingTerminate(quint16);
    void closePortForwardingTerminate();
    void sendFileTerminate(QString);

public slots:
    void tx_data(qint64 len);
    void rx_data(qint64 len);
    void setKeys(const QString &publicKey, const QString &privateKey);
    bool loadKnownHosts(const QString &file);
    int connectSshToHost(const QString & username, const QString & hostname, quint16 port = 22, bool lock = true, bool checkHostKey = false, unsigned int retry = 5);
    QString runCommand(QString command);
    void disconnectSshFromHost();
    quint16 openLocalPortForwarding(QString servicename, quint16 port, quint16 bind);
    quint16 openRemotePortForwarding(QString servicename, quint16 port);
    void closePortForwarding(QString servicename);

    QString sendFile(QString src, QString dst);

private slots:
    void _readyRead();
    void _connected();
    void _disconnected();
    void _delaydErrorEmit();
    void _reset();
    void _getLastError();
    void _tcperror(QAbstractSocket::SocketError err);
    void _cntRate();
    void _sendKeepAlive();
};

#endif
