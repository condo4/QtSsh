#include "sshclient.h"
#include <QTemporaryFile>
#include <QDir>
#include <QThread>
#include <QEventLoop>
#include <QCoreApplication>
#include "sshtunnelin.h"
#include "sshtunneloutsrv.h"
#include "sshprocess.h"
#include "sshscpsend.h"
#include "sshsftp.h"
#include "cerrno"

Q_LOGGING_CATEGORY(sshclient, "ssh.client", QtWarningMsg)

static ssize_t qt_callback_libssh_recv(int socket,void *buffer, size_t length,int flags, void **abstract)
{
    Q_UNUSED(socket);
    Q_UNUSED(flags);

    QTcpSocket * c = reinterpret_cast<QTcpSocket *>(* abstract);
    qint64 r = c->read(reinterpret_cast<char *>(buffer), length);
    if (r == 0)
    {
        return -EAGAIN;
    }
    return static_cast<ssize_t>(r);
}

static ssize_t qt_callback_libssh_send(int socket,const void * buffer, size_t length,int flags, void ** abstract)
{
    Q_UNUSED(socket);
    Q_UNUSED(flags);

    QTcpSocket * c = reinterpret_cast<QTcpSocket *>(* abstract);
    qint64 r = c->write(reinterpret_cast<const char *>(buffer), length);
    if (r == 0)
    {
        return -EAGAIN;
    }
    return static_cast<ssize_t>(r);
}

bool SshClient::getSshConnected() const
{
    return m_sshConnected;
}

QString SshClient::getName() const
{
    return m_name;
}

SshClient::SshClient(const QString &name, QObject * parent):
    QObject(parent),
    m_session(nullptr),
    m_knownHosts(nullptr),
    m_name(name),
    m_socket(this),
    m_port(0),
    m_errorcode(0),
    m_sshConnected(false),
    m_errorMessage(QString()),
    m_cntTxData(0),
    m_cntRxData(0)
{
    qCDebug(sshclient, "%s: Enter in constructor", qPrintable(m_name));

    connect(&m_socket,   SIGNAL(disconnected()),                      this, SLOT(_disconnected()));
    connect(&m_socket,   SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_tcperror(QAbstractSocket::SocketError)));
    connect(&m_cntTimer, SIGNAL(timeout()),                           this, SLOT(_cntRate()));
    connect(&m_keepalive,SIGNAL(timeout()),                           this, SLOT(_sendKeepAlive()));

    Q_ASSERT(qssh2_init(0) == 0);
    m_session = qssh2_session_init_ex(nullptr, nullptr, nullptr, reinterpret_cast<void *>(&m_socket));

    qssh2_session_callback_set(m_session, LIBSSH2_CALLBACK_RECV,reinterpret_cast<void*>(& qt_callback_libssh_recv));
    qssh2_session_callback_set(m_session, LIBSSH2_CALLBACK_SEND,reinterpret_cast<void*>(& qt_callback_libssh_send));
    Q_ASSERT(m_session);
    m_knownHosts = qssh2_knownhost_init(m_session);
    Q_ASSERT(m_knownHosts);
    qssh2_session_set_blocking(m_session, 0);

    m_cntTimer.setInterval(1000);
    m_cntTimer.start();
}

SshClient::~SshClient()
{
    qCDebug(sshclient, "%s: Enter in destructor", qPrintable(m_name));
    disconnectFromHost();
    qCDebug(sshclient, "%s: Leave destructor", qPrintable(m_name));
}


LIBSSH2_SESSION *SshClient::session()
{
    return m_session;
}

bool SshClient::channelReady()
{
    return m_sshConnected;
}

bool SshClient::loopWhileBytesWritten(int msecs)
{

    QEventLoop wait;
    QTimer timeout;
    bool written;
    auto con1 = QObject::connect(&m_socket, &QTcpSocket::bytesWritten, [this, &written, &wait]()
    {
        qCDebug(sshclient, "%s: BytesWritten", qPrintable(m_name));
        written = true;
        wait.quit();
    });
    auto con2 = QObject::connect(&timeout, &QTimer::timeout, [this, &written, &wait]()
    {
        qCWarning(sshclient, "%s: Bytes Write Timeout", qPrintable(m_name));
        written = false;
        wait.quit();
    });
    auto con3 = QObject::connect(&m_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [this, &written, &wait]()
    {
        qCWarning(sshclient, "%s: Socket Error", qPrintable(m_name));
        written = false;
        wait.quit();
    });
    timeout.start(msecs); /* Timeout 10s */
    wait.exec();
    QObject::disconnect(con1);
    QObject::disconnect(con2);
    QObject::disconnect(con3);
    return written;
}

quint16 SshClient::openLocalPortForwarding(const QString &servicename, quint16 port, quint16 bind)
{
    if(m_channels.contains(servicename))
    {
        return m_channels.value(servicename)->localPort();
    }

    SshTunnelIn *tunnel = new SshTunnelIn(this, servicename, port, bind);
    if(!tunnel->valid())
    {
        qCWarning(sshclient, "SshTunnelIn creation failed");
        return 0;
    }
    m_channels.insert(servicename, tunnel);
    return tunnel->localPort();
}

quint16 SshClient::openRemotePortForwarding(const QString &servicename, quint16 port)
{
    if(m_channels.contains(servicename))
    {
        return m_channels.value(servicename)->localPort();
    }

    SshChannel *tunnel = qobject_cast<SshChannel *>(new SshTunnelOutSrv(this, servicename, port));
    m_channels.insert(servicename, tunnel);
    return tunnel->localPort();
}

void SshClient::closePortForwarding(const QString &servicename)
{
    qCDebug(sshclient, "%s: SshClient::closePortForwarding(%s)", qPrintable(m_name), qPrintable(servicename));

    if(m_channels.contains(servicename))
    {
        SshChannel *tunnel = m_channels.value(servicename);
        m_channels.remove(servicename);
        delete tunnel;
    }
}

QString SshClient::runCommand(const QString &command)
{
    QString res;
    SshProcess *sshProcess = new SshProcess(this);
    res = sshProcess->runCommand(command);
    return res;
}

QString SshClient::sendFile(const QString &src, const QString &dst)
{
    SshScpSend *sender = new SshScpSend(this);
    QString d = sender->send(src, dst);
    delete sender;
    return d;
}


int SshClient::connectToHost(const QString & user, const QString & host, quint16 port)
{
    if(m_sshConnected) {
        qCCritical(sshclient, "%s: Allready connected", qPrintable(m_name));
        return 0;
    }
    QEventLoop wait;
    QTimer timeout;
    m_hostname = host;
    m_username = user;
    m_port = port;
    m_errorcode = 0;

    qCDebug(sshclient, "%s: trying to connect to host (%s:%i)", qPrintable(m_name), qPrintable(m_hostname), m_port);

    m_socket.connectToHost(m_hostname, m_port);
    if(!m_socket.waitForConnected(60000))
    {
        qCDebug(sshclient, "%s: Failed to connect socket", qPrintable(m_name));
        return -1;
    }

    qCDebug(sshclient, "%s: ssh socket connected", qPrintable(m_name));

    /* Socket is ready, now start to initialize Ssh Session */
    int sock = static_cast<int>(m_socket.socketDescriptor());
    int ret = qssh2_session_handshake(m_session, sock);
    if(ret)
    {
        qCCritical(sshclient, "%s: Handshake error %s", qPrintable(m_name), sshErrorToString(ret));
        m_socket.disconnectFromHost();
        m_socket.waitForDisconnected(60000);
        return -1;
    }

    /* Ssh session initialized */
    size_t len;
    int type;
    const char * fingerprint = qssh2_session_hostkey(m_session, &len, &type);

    if(fingerprint == nullptr)
    {
        qCCritical(sshclient, "%s: Fingerprint error", qPrintable(m_name));
        m_socket.disconnectFromHost();
        m_socket.waitForDisconnected(60000);
        return -1;
    }

    m_hostKey.hash = QByteArray(qssh2_hostkey_hash(m_session,LIBSSH2_HOSTKEY_HASH_MD5), 16);
    switch (type)
    {
        case LIBSSH2_HOSTKEY_TYPE_RSA:
            m_hostKey.type=SshKey::Rsa;
            break;
        case LIBSSH2_HOSTKEY_TYPE_DSS:
            m_hostKey.type=SshKey::Dss;
            break;
        default:
            m_hostKey.type=SshKey::UnknownType;
    }

    m_hostKey.key = QByteArray(fingerprint, static_cast<int>(len));
    struct qssh2_knownhost *khost;
    qssh2_knownhost_check(m_knownHosts, m_hostname.toStdString().c_str(), fingerprint, len, LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &khost);

    QByteArray username = m_username.toLocal8Bit();
    char * alist = nullptr;

    alist = qssh2_userauth_list(m_session, username.data(), static_cast<unsigned int>(username.length()));
    if(alist == nullptr)
    {
        int ret = qssh2_session_last_error(m_session, nullptr, nullptr, 0);
        qCDebug(sshclient, "%s: Failed to authenticate: %s", qPrintable(m_name), qPrintable(sshErrorToString(ret)));
    }

    if (alist == nullptr && !qssh2_userauth_authenticated(m_session))
    {
        /* Autentication Error */
        qCCritical(sshclient, "%s: Authentication error %s", qPrintable(m_name), qPrintable(sshErrorToString(ret)));
        m_socket.disconnectFromHost();
        m_socket.waitForDisconnected(60000);
        return -1;
    }

    QByteArrayList methodes = QByteArray(alist).split(',');

    while(!qssh2_userauth_authenticated(m_session) && methodes.length() > 0)
    {
        if(methodes.contains("publickey"))
        {
            qCDebug(sshclient, "%s: Trying authentication by publickey", qPrintable(m_name));

            ret = qssh2_userauth_publickey_frommemory(
                            m_session,
                            m_username.toStdString().c_str(),
                            static_cast<size_t>(m_username.length()),
                            m_publicKey.toStdString().c_str(),
                            static_cast<size_t>(m_publicKey.length()),
                            m_privateKey.toStdString().c_str(),
                            static_cast<size_t>(m_privateKey.length()),
                            m_passphrase.toStdString().c_str()
                    );

            if(ret)
            {
                qCDebug(sshclient, "%s: Failed to userauth_publickey: %s", qPrintable(m_name), qPrintable(sshErrorToString(ret)));
                methodes.removeAll("publickey");
            }
        }
        else if(methodes.contains("password"))
        {
            QByteArray username = m_username.toLatin1();
            QByteArray passphrase = m_passphrase.toLatin1();

            ret = qssh2_userauth_password_ex(m_session,
                                                     username.data(),
                                                     username.length(),
                                                     passphrase.data(),
                                                     passphrase.length());
            if(ret)
            {
                qCDebug(sshclient, "%s: Failed to userauth_password: %s", qPrintable(m_name), qPrintable(sshErrorToString(ret)));
                methodes.removeAll("password");
            }
        }
        else
        {
            qCCritical(sshclient, "%s: Not manage %s authentication", qPrintable(m_name), qPrintable(methodes.at(0)));
            methodes.removeAll(methodes.at(0));
        }
    }

    if(!qssh2_userauth_authenticated(m_session))
    {
        /* Autentication Error */
        qCCritical(sshclient, "%s: Authentication error not more available methodes", qPrintable(m_name));
        m_socket.disconnectFromHost();
        m_socket.waitForDisconnected(60000);
        return -1;
    }

    qCDebug(sshclient, "%s: Connected and authenticated", qPrintable(m_name));
    QObject::connect(&m_socket, &QAbstractSocket::readyRead, this, &SshClient::_readyRead);

    m_keepalive.setInterval(10000);
    m_keepalive.start();
    qssh2_keepalive_config(m_session, 1, 5);
    m_sshConnected = true;
    return 0;
}

void SshClient::_readyRead()
{
    emit sshDataReceived();
}

void SshClient::disconnectFromHost()
{
    if(!m_sshConnected) return;

    QEventLoop wait;

    /* Close all Opened Channels */
    for(SshChannel *ch: m_channels)
    {
        delete ch;
    }
    m_channels.clear();

    m_keepalive.stop();
    emit sshReset();

    QObject::disconnect(&m_socket, &QAbstractSocket::readyRead, this, &SshClient::_readyRead);

    if (m_knownHosts)
    {
        qssh2_knownhost_free(m_knownHosts);
    }


    if (m_session)
    {
        qssh2_session_free(m_session);
    }

    m_errorcode = 0;
    m_errorMessage = QString();
    m_sshConnected = false;
    m_session = nullptr;

    disconnect(&m_socket, SIGNAL(disconnected()), this, SLOT(_disconnected()));
    if(m_socket.state() != QTcpSocket::UnconnectedState)
    {
        m_socket.disconnectFromHost();
        if(m_socket.state() != QTcpSocket::UnconnectedState)
        {
            m_socket.waitForDisconnected();
        }
    }
    m_socket.close();
}

void SshClient::setPassphrase(const QString & pass)
{
    m_passphrase = pass;
}

void SshClient::setKeys(const QString &publicKey, const QString &privateKey)
{
    m_publicKey  = publicKey;
    m_privateKey = privateKey;
}

bool SshClient::loadKnownHosts(const QString & file)
{
    bool res = (qssh2_knownhost_readfile(m_knownHosts, qPrintable(file), LIBSSH2_KNOWNHOST_FILE_OPENSSH) == 0);
    return (res);
}

bool SshClient::saveKnownHosts(const QString & file)
{
    bool res = (qssh2_knownhost_writefile(m_knownHosts, qPrintable(file), LIBSSH2_KNOWNHOST_FILE_OPENSSH) == 0);
    return res;
}

bool SshClient::addKnownHost(const QString & hostname,const SshKey & key)
{
    bool ret;
    int typemask = LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW;
    switch (key.type)
    {
        case SshKey::Dss:
            typemask |= LIBSSH2_KNOWNHOST_KEY_SSHDSS;
            break;
        case SshKey::Rsa:
            typemask |= LIBSSH2_KNOWNHOST_KEY_SSHRSA;
            break;
        case SshKey::UnknownType:
            return false;
    };
    ret = (qssh2_knownhost_add(m_knownHosts, qPrintable(hostname), nullptr, key.key.data(), static_cast<size_t>(key.key.size()), typemask, nullptr));
    return ret;
}

QString SshClient::banner()
{
    return QString(qssh2_session_banner_get(m_session));
}

void SshClient::waitSocket()
{
    m_socket.waitForReadyRead();
}


void SshClient::_tcperror(QAbstractSocket::SocketError err)
{
    if(err == QAbstractSocket::ConnectionRefusedError)
    {
        m_errorcode = LIBSSH2_ERROR_BAD_SOCKET;
        qCCritical(sshclient, "%s: Connection Refused Error", qPrintable(m_name));
    }
    else
    {
        qCCritical(sshclient, "%s: failed to connect session tcp socket, err=%i", qPrintable(m_name), err);
    }
}

void SshClient::_cntRate()
{
    emit xfer_rate(m_cntTxData, m_cntRxData);
    m_cntRxData = 0;
    m_cntTxData = 0;
}

void SshClient::_sendKeepAlive()
{
    int keepalive;
    qssh2_keepalive_send(m_session, &keepalive);
}

void SshClient::askDisconnect()
{
    /* Close all Opened Channels */
    for(SshChannel *ch: m_channels)
    {
        delete ch;
    }
    m_channels.clear();

    qCDebug(sshclient, "%s: reset", qPrintable(m_name));
    m_keepalive.stop();
    emit sshReset();

    if (m_knownHosts)
    {
        qssh2_knownhost_free(m_knownHosts);
    }

    if (m_session)
    {
        qssh2_session_disconnect(m_session, "good bye!");
        qssh2_session_free(m_session);
    }
    m_errorcode = 0;
    m_errorMessage = QString();

    m_session = qssh2_session_init_ex(nullptr, nullptr, nullptr, reinterpret_cast<void *>(this));

    qssh2_session_callback_set(m_session, LIBSSH2_CALLBACK_RECV,reinterpret_cast<void*>(& qt_callback_libssh_recv));
    qssh2_session_callback_set(m_session, LIBSSH2_CALLBACK_SEND,reinterpret_cast<void*>(& qt_callback_libssh_send));
    Q_ASSERT(m_session);
    m_knownHosts = qssh2_knownhost_init(m_session);
    Q_ASSERT(m_knownHosts);
    qssh2_session_set_blocking(m_session, 0);
    m_socket.disconnectFromHost();
    qCDebug(sshclient, "%s: reset disconnected", qPrintable(m_name));
}

void SshClient::_disconnected()
{
    m_keepalive.stop();

    qCWarning(sshclient, "%s: unexpected shutdown", qPrintable(m_name));
    emit unexpectedDisconnection();
    askDisconnect();

    m_sshConnected = false;
    emit disconnected();
}

void SshClient::_getLastError()
{
    char * msg;
    int len = 0;
    m_errorcode = qssh2_session_last_error(m_session, &msg, &len, 0);
    m_errorMessage = QString::fromLocal8Bit(QByteArray::fromRawData(msg, len));
}

