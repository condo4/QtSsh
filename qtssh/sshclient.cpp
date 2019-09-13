#include "sshclient.h"
#include <QTemporaryFile>
#include <QDir>
#include <QEventLoop>
#include <QDateTime>
#include <QCoreApplication>
#include "sshtunnelin.h"
#include "sshtunnelout.h"
#include "sshprocess.h"
#include "sshscpsend.h"
#include "sshscpget.h"
#include "sshsftp.h"
#include "cerrno"

Q_LOGGING_CATEGORY(sshclient, "ssh.client", QtWarningMsg)

#if !defined(MAX_LOST_KEEP_ALIVE)

/*
 * Maximum keep alive cycle (generally 5s) before connection is
 * considered as lost
 * Default: 6 (30s)
 */
#define MAX_LOST_KEEP_ALIVE 6
#endif

int SshClient::s_nbInstance = 0;

static ssize_t qt_callback_libssh_recv(int socket,void *buffer, size_t length,int flags, void **abstract)
{
    Q_UNUSED(socket)
    Q_UNUSED(flags)

    QTcpSocket * c = reinterpret_cast<QTcpSocket *>(* abstract);
    qint64 r = c->read(reinterpret_cast<char *>(buffer), static_cast<qint64>(length));
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
    qint64 r = c->write(reinterpret_cast<const char *>(buffer), static_cast<qint64>(length));
    if (r == 0)
    {
        return -EAGAIN;
    }
    return static_cast<ssize_t>(r);
}

SshClient::SshClient(const QString &name, QObject * parent):
    QObject(parent),
    m_name(name),
    m_socket(this)
{
    connect(&m_socket,   SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_tcperror(QAbstractSocket::SocketError)));

    if(s_nbInstance == 0)
    {
        qCDebug(sshclient) << m_name << ": libssh2_init()";
        Q_ASSERT(libssh2_init(0) == 0);
    }
    ++s_nbInstance;

    qCDebug(sshclient) << m_name << ": created " << this;
}

SshClient::~SshClient()
{
    qCDebug(sshclient) << m_name << ": SshClient::~SshClient() " << this;
    disconnectFromHost();
    --s_nbInstance;
    if(s_nbInstance == 0)
    {
        qCDebug(sshclient) << m_name << ": libssh2_exit()";
        libssh2_exit();
    }
    qCDebug(sshclient) << m_name << ": destroyed";
}


bool SshClient::getSshConnected() const
{
    return m_sshConnected;
}

QString SshClient::getName() const
{
    return m_name;
}

bool SshClient::takeChannelCreationMutex(void *identifier)
{
    if ( ! channelCreationInProgress.tryLock() && currentLockerForChannelCreation != identifier )
    {
        qCDebug(sshclient) << "takeChannelCreationMutex another channel is already been created, have to wait";
        return false;
    }
    currentLockerForChannelCreation = identifier;
    return true;
}

void SshClient::releaseChannelCreationMutex(void *identifier)
{
    if ( currentLockerForChannelCreation == identifier )
    {
        channelCreationInProgress.unlock();
        currentLockerForChannelCreation = nullptr;
    }
    else
    {
        qCCritical(sshclient) << "Trying to release channel mutex but it doesn't host it";
    }
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

QString SshClient::runCommand(const QString &command)
{
    QString res;
    QScopedPointer<SshProcess> sshProcess(new SshProcess(command, this));
    res = sshProcess->runCommand(command);
    return res;
}

QString SshClient::getFile(const QString &source, const QString &dest)
{
    QString res;
    QScopedPointer<SshScpGet> sshScpGet(new SshScpGet(source, this));
    res = sshScpGet->get(source, dest);
    return res;
}

QString SshClient::sendFile(const QString &src, const QString &dst)
{
    QScopedPointer<SshScpSend> sender(new SshScpSend(src, this));
    QString d = sender->send(src, dst);
    return d;
}

QSharedPointer<SshSFtp> SshClient::getSFtp(const QString &name)
{
    for(QWeakPointer<SshChannel> &w: m_channels)
    {
        if(!w.isNull())
        {
            QSharedPointer<SshChannel> c = w.toStrongRef();
            if(name == c->name())
            {
                QSharedPointer<SshSFtp> ftp = qSharedPointerDynamicCast<SshSFtp>(c);
                if(!ftp.isNull())
                {
                    return ftp;
                }
            }
        }
    }
    QSharedPointer<SshSFtp> ftp(new SshSFtp(name, this));
    m_channels.append(ftp.toWeakRef());
    return ftp;
}

QSharedPointer<SshTunnelIn> SshClient::getTunnelIn(const QString &name, quint16 localport, quint16 remoteport, QString host)
{
    for(QWeakPointer<SshChannel> &w: m_channels)
    {
        if(!w.isNull())
        {
            QSharedPointer<SshChannel> c = w.toStrongRef();
            if(name == c->name())
            {
                QSharedPointer<SshTunnelIn> in = qSharedPointerDynamicCast<SshTunnelIn>(c);
                if(!in.isNull())
                {
                    return in;
                }
            }
        }
    }
    QSharedPointer<SshTunnelIn> in(new SshTunnelIn(this, name, localport, remoteport, host));
    m_channels.append(in.toWeakRef());
    return in;
}

QSharedPointer<SshTunnelOut> SshClient::getTunnelOut(const QString &name, quint16 port)
{
    for(QWeakPointer<SshChannel> &w: m_channels)
    {
        if(!w.isNull())
        {
            QSharedPointer<SshChannel> c = w.toStrongRef();
            if(name == c->name())
            {
                QSharedPointer<SshTunnelOut> out = qSharedPointerDynamicCast<SshTunnelOut>(c);
                if(!out.isNull())
                {
                    return out;
                }
            }
        }
    }
    QSharedPointer<SshTunnelOut> out(new SshTunnelOut(this, name, port));
    out->setSharedPointer(out);
    m_channels.append(out.toWeakRef());
    return out;
}

int SshClient::connectToHost(const QString & user, const QString & host, quint16 port, QByteArrayList methodes)
{
    QEventLoop waitnextframe;
    connect(&m_socket,   &QTcpSocket::stateChanged, this, &SshClient::_stateChanged);
    connect(&m_socket,   &QTcpSocket::disconnected, this, &SshClient::_disconnected);
    connect(&m_keepalive,&QTimer::timeout,          this, &SshClient::_sendKeepAlive);

    QObject::connect(&m_socket, &QTcpSocket::readyRead, &waitnextframe, &QEventLoop::quit);
    QObject::connect(&m_socket, &QTcpSocket::disconnected, &waitnextframe, &QEventLoop::quit);

    m_hostname = host;
    m_port = port;
    m_username = user;
    qCDebug(sshclient) << m_name << ": connectToHost(" << m_hostname << "," << m_port << ") with login " << user;
    if(m_sshConnected) {
        qCCritical(sshclient) << m_name << "Allready connected";
        return 0;
    }

    QEventLoop wait;
    QTimer timeout;
    m_errorcode = 0;

    m_socket.connectToHost(m_hostname, m_port);
    if(!m_socket.waitForConnected(60000))
    {
        qCDebug(sshclient) << m_name << "Failed to connect socket";
        return -1;
    }

    qCDebug(sshclient) << m_name << ": ssh socket connected";

    /* Socket is ready, now start to initialize Ssh Session */
    int sock = static_cast<int>(m_socket.socketDescriptor());


    m_errorcode = 0;
    m_errorMessage = QString();
    m_session = libssh2_session_init_ex(nullptr, nullptr, nullptr, reinterpret_cast<void *>(&m_socket));
    libssh2_session_callback_set(m_session, LIBSSH2_CALLBACK_RECV,reinterpret_cast<void*>(& qt_callback_libssh_recv));
    libssh2_session_callback_set(m_session, LIBSSH2_CALLBACK_SEND,reinterpret_cast<void*>(& qt_callback_libssh_send));
    Q_ASSERT(m_session);

    libssh2_session_set_blocking(m_session, 0);

    m_knownHosts = libssh2_knownhost_init(m_session);
    Q_ASSERT(m_knownHosts);

    if(m_knowhostFiles.size())
    {
        _loadKnownHosts(m_knowhostFiles);
    }

    int ret = qssh2_session_handshake(m_session, sock);
    if(ret)
    {
        qCCritical(sshclient) << m_name << "Handshake error" << sshErrorToString(ret);
        m_socket.disconnectFromHost();
        if(m_socket.state() != QTcpSocket::UnconnectedState)
            m_socket.waitForDisconnected(60000);
        return -1;
    }

    /* Ssh session initialized */
    size_t len;
    int type;
    const char * fingerprint = libssh2_session_hostkey(m_session, &len, &type);

    if(fingerprint == nullptr)
    {
        qCCritical(sshclient) << m_name << "Fingerprint error";
        m_socket.disconnectFromHost();
        if(m_socket.state() != QTcpSocket::UnconnectedState)
            m_socket.waitForDisconnected(60000);
        return -1;
    }

    m_hostKey.hash = QByteArray(libssh2_hostkey_hash(m_session,LIBSSH2_HOSTKEY_HASH_MD5), 16);
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
    struct libssh2_knownhost *khost;
    libssh2_knownhost_check(m_knownHosts, m_hostname.toStdString().c_str(), fingerprint, len, LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &khost);


    if(methodes.length() == 0)
    {
        QByteArray username = m_username.toLocal8Bit();
        char * alist = nullptr;

        qCDebug(sshclient) << m_name << ": ssh start authentication userauth_list";

        while(alist == nullptr)
        {
            alist = libssh2_userauth_list(m_session, username.data(), static_cast<unsigned int>(username.length()));
            if(alist == nullptr)
            {
                int ret = libssh2_session_last_error(m_session, nullptr, nullptr, 0);
                if(ret == LIBSSH2_ERROR_EAGAIN)
                {
                    qCDebug(sshclient) << m_name << ": again";
                    waitnextframe.exec();
                    if(m_socket.state() != QAbstractSocket::ConnectedState)
                        break;
                    continue;
                }
                qCDebug(sshclient) << m_name << ": Failed to authenticate:" << sshErrorToString(ret);
                break;
            }
            qCDebug(sshclient) << m_name << ": ssh start authentication userauth_list: " << alist;
        }

        if (alist == nullptr && !libssh2_userauth_authenticated(m_session))
        {
            /* Autentication Error */
            qCCritical(sshclient) << m_name << "Authentication error" << sshErrorToString(ret);
            m_socket.disconnectFromHost();
            if(m_socket.state() != QTcpSocket::UnconnectedState)
                m_socket.waitForDisconnected(60000);
            return -1;
        }

        methodes = QByteArray(alist).split(',');
    }

    while(!libssh2_userauth_authenticated(m_session) && methodes.length() > 0)
    {
        if(methodes.contains("publickey"))
        {
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
                                                     static_cast<unsigned int>(username.length()),
                                                     passphrase.data(),
                                                     static_cast<unsigned int>(passphrase.length()));
            if(ret)
            {
                qCDebug(sshclient) << m_name << ": Failed to userauth_password:" << sshErrorToString(ret);
                methodes.removeAll("password");
            }
        }
        else
        {
            qCCritical(sshclient) << m_name << ": Not manage" << methodes.at(0) << "authentication";
            methodes.removeAll(methodes.at(0));
        }
    }

    if(!libssh2_userauth_authenticated(m_session))
    {
        /* Autentication Error */
        qCDebug(sshclient) << m_name << ": Authentication error not more available methodes";
        m_socket.disconnectFromHost();
        if(m_socket.state() != QTcpSocket::UnconnectedState)
            m_socket.waitForDisconnected(60000);
        return -1;
    }

    qCDebug(sshclient) << m_name << ": Connected and authenticated";
    QObject::connect(&m_socket, &QAbstractSocket::readyRead, this, &SshClient::_readyRead);

    m_keepalive.setInterval(1000);
    m_keepalive.setSingleShot(true);
    m_keepalive.start();
    libssh2_keepalive_config(m_session, 1, 5);
    m_sshConnected = true;
    return 0;
}

void SshClient::_readyRead()
{
    m_lastProofOfLive = QDateTime::currentMSecsSinceEpoch();
    emit sshDataReceived();
}

void SshClient::disconnectFromHost()
{
    if(!m_sshConnected) return;

    qCDebug(sshclient) << m_name << ": disconnectFromHost()";

    /* Disconnect socket */
    QObject::disconnect(&m_socket, &QAbstractSocket::readyRead,    this, &SshClient::_readyRead);
    QObject::disconnect(&m_socket, &QAbstractSocket::disconnected, this, &SshClient::_disconnected);
    QObject::disconnect(&m_keepalive,&QTimer::timeout,             this, &SshClient::_sendKeepAlive);

    _sshClientClose();
    _sshClientFree();

    if(m_socket.state() != QTcpSocket::UnconnectedState)
    {
        m_socket.disconnectFromHost();
        if(m_socket.state() != QTcpSocket::UnconnectedState)
        {
            m_socket.waitForDisconnected();
        }
    }
    m_socket.close();

    m_sshConnected = false;
    emit disconnected();

    qCDebug(sshclient) << m_name << ": disconnected()";
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

bool SshClient::_loadKnownHosts(const QString & file)
{
    bool res = (libssh2_knownhost_readfile(m_knownHosts, qPrintable(file), LIBSSH2_KNOWNHOST_FILE_OPENSSH) == 0);
    return (res);
}

bool SshClient::saveKnownHosts(const QString & file)
{
    bool res = (libssh2_knownhost_writefile(m_knownHosts, qPrintable(file), LIBSSH2_KNOWNHOST_FILE_OPENSSH) == 0);
    return res;
}

void SshClient::setKownHostFile(const QString &file)
{
    m_knowhostFiles = file;
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
    ret = (libssh2_knownhost_add(m_knownHosts, qPrintable(hostname), nullptr, key.key.data(), static_cast<size_t>(key.key.size()), typemask, nullptr));
    return ret;
}

QString SshClient::banner()
{
    return QString(libssh2_session_banner_get(m_session));
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
    }
    qCDebug(sshclient) << m_name << ": Error " << err;
    emit error(err);
}

void SshClient::_sendKeepAlive()
{
    int keepalive = 0;
    if(m_session)
    {
        int ret = libssh2_keepalive_send(m_session, &keepalive);
        if(ret == LIBSSH2_ERROR_SOCKET_SEND)
        {
            qCWarning(sshclient) << m_name << ": Connection I/O error !!!";
            m_socket.disconnectFromHost();
        }

        if(((QDateTime::currentMSecsSinceEpoch() - m_lastProofOfLive) / 1000) > (MAX_LOST_KEEP_ALIVE * keepalive))
        {
            qCWarning(sshclient) << m_name << ": Connection lost !!!";
            m_socket.disconnectFromHost();
        }
        m_keepalive.start(keepalive * 1000);
    }
}

void SshClient::_stateChanged(QAbstractSocket::SocketState socketState)
{
    qCDebug(sshclient) << m_name << ": State changed " << socketState;
    emit stateChanged(socketState);
}

void SshClient::_disconnected()
{
    qCWarning(sshclient) << m_name << "Unexpected disconnection";
    emit sshDisconnected();
    QObject::disconnect(&m_socket, &QAbstractSocket::readyRead,    this, &SshClient::_readyRead);
    QObject::disconnect(&m_socket, &QAbstractSocket::disconnected, this, &SshClient::_disconnected);
    QObject::disconnect(&m_keepalive,&QTimer::timeout,             this, &SshClient::_sendKeepAlive);
    _sshClientFree();
    emit disconnected();
}

void SshClient::_getLastError()
{
    char * msg;
    int len = 0;
    m_errorcode = libssh2_session_last_error(m_session, &msg, &len, 0);
    m_errorMessage = QString::fromLocal8Bit(QByteArray::fromRawData(msg, len));
}


void SshClient::_sshClientClose()
{
    if (!m_session) return;

    /* Close all Opened Channels */
    qCDebug(sshclient) << m_name << ": close all channels";
    for(const QWeakPointer<SshChannel> &wchannel: m_channels)
    {
        if(!wchannel.isNull())
        {
            QSharedPointer<SshChannel> channel = wchannel.toStrongRef();
            channel->close();
        }
    }
    m_channels.clear();
    qCDebug(sshclient) << m_name << ": no more channel registered";

    /* Stop keepalive */
    m_keepalive.stop();

    qssh2_session_disconnect(m_session, "good bye!");
}

void SshClient::_sshClientFree()
{
    /* Disconnect session */
    if (m_knownHosts)
    {
        libssh2_knownhost_free(m_knownHosts);
        m_knownHosts = nullptr;
    }

    if (m_session)
    {
        qCDebug(sshclient) << m_name << ": Destroy SSH session";
        qssh2_session_free(m_session);
        m_session = nullptr;
    }
}
