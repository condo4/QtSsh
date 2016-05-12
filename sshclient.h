#ifndef QT_SSH_CLIENT_H
#define QT_SSH_CLIENT_H

#include <QObject>
#include <QList>
#include <QTcpSocket>
#include <QTimer>
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


class  SshClient : public QTcpSocket
{
    Q_OBJECT

public:
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
    enum KnownHostsFormat {
        OpenSslFormat
    };
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

    SshClient(QObject * parent = NULL);
    ~SshClient();

    int connectToHost(const QString & username,const QString & hostname, quint16 port = 22, bool lock = true, bool checkHostKey = false, unsigned int retry = 5);
    void disconnectFromHost();
    void setPassphrase(const QString & pass);
    void setKeys(const QString &publicKey, const QString &privateKey);

    bool loadKnownHosts(const QString &file, KnownHostsFormat c = OpenSslFormat);
    bool saveKnownHosts(const QString &file, KnownHostsFormat c = OpenSslFormat) const;
    bool addKnownHost  (const QString &hostname, const SshKey &key);

    quint16 sshSocketLocalPort();

    QString hostName() const;
    SshKey hostKey() const;

    LIBSSH2_SESSION *session();

    bool channelReady();

signals:
    void sshConnected();
    void sshDisconnected();
    void sshError(SshClient::Error error);
    void sshAuthenticationRequired(QList<SshClient::AuthenticationMethod> availableMethods);
    void xfer_rate(qint64 tx, qint64 rx);
    void sshDataReceived();
    void sshReset();
    void _connectionTerminate();

public slots:
    void tx_data(qint64 len);
    void rx_data(qint64 len);

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

private:
    // libssh2 stuff
    LIBSSH2_SESSION    * _session;
    LIBSSH2_KNOWNHOSTS * _knownHosts;

    SshState _state;

    int _port;
    int _errorcode;

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
};

#endif
