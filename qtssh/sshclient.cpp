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

//#define DEBUG_SSHCLIENT

static ssize_t qt_callback_libssh_recv(int socket,void *buffer, size_t length,int flags, void **abstract)
{
    Q_UNUSED(socket);
    Q_UNUSED(flags);

    QTcpSocket * c = reinterpret_cast<QTcpSocket *>(* abstract);
    int r = c->read(reinterpret_cast<char *>(buffer), length);
    if (r == 0)
    {
        return -EAGAIN;
    }
    return r;
}

static ssize_t qt_callback_libssh_send(int socket,const void * buffer, size_t length,int flags, void ** abstract)
{
    Q_UNUSED(socket);
    Q_UNUSED(flags);

    QTcpSocket * c = reinterpret_cast<QTcpSocket *>(* abstract);
    int r = c->write(reinterpret_cast<const char *>(buffer), length);
    if (r == 0)
    {
        return -EAGAIN;
    }
    return r;
}

bool SshClient::getSshConnected() const
{
    return _sshConnected;
}

QString SshClient::getName() const
{
    return _name;
}

SshClient::SshClient(QString name, QObject * parent):
    QObject(parent),
    _name(name),
    _session(nullptr),
    _knownHosts(nullptr),
    _sftp(nullptr),
    _socket(this),
    _errorcode(0),
    _sshConnected(false),
    _errorMessage(QString()),
    _cntTxData(0),
    _cntRxData(0)
{
#if defined(DEBUG_SSHCLIENT) || defined(DEBUG_THREAD)
    qDebug() << "DEBUG : SshClient("<< _name << ") : Enter in constructor, @" << this << " in " << QThread::currentThread() << " (" << QThread::currentThreadId() << ")";
#endif

    connect(&_socket,   SIGNAL(disconnected()),                      this, SLOT(_disconnected()));
    connect(&_socket,   SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_tcperror(QAbstractSocket::SocketError)));
    connect(&_cntTimer, SIGNAL(timeout()),                           this, SLOT(_cntRate()));
    connect(&_keepalive,SIGNAL(timeout()),                           this, SLOT(_sendKeepAlive()));

    Q_ASSERT(qssh2_init(0) == 0);
    _session = qssh2_session_init_ex(nullptr, nullptr, nullptr, reinterpret_cast<void *>(&_socket));

    qssh2_session_callback_set(_session, LIBSSH2_CALLBACK_RECV,reinterpret_cast<void*>(& qt_callback_libssh_recv));
    qssh2_session_callback_set(_session, LIBSSH2_CALLBACK_SEND,reinterpret_cast<void*>(& qt_callback_libssh_send));
    Q_ASSERT(_session);
    _knownHosts = qssh2_knownhost_init(_session);
    Q_ASSERT(_knownHosts);
    qssh2_session_set_blocking(_session, 0);

    _cntTimer.setInterval(1000);
    _cntTimer.start();
}

SshClient::~SshClient()
{
#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshClient("<< _name << ") : Enter in destructor, @"<< this;
#endif

    disconnectFromHost();

#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshClient("<< _name << ") : Quit destructor";
#endif
}


LIBSSH2_SESSION *SshClient::session()
{
    return _session;
}

bool SshClient::channelReady()
{
    return _sshConnected;
}

bool SshClient::loopWhileBytesWritten(int msecs)
{

    QEventLoop wait;
    QTimer timeout;
    bool written;
    auto con1 = QObject::connect(&_socket, &QTcpSocket::bytesWritten, [&written, &wait]()
    {
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshClient() : BytesWritten";
#endif
        written = true;
        wait.quit();
    });
    auto con2 = QObject::connect(&timeout, &QTimer::timeout, [&written, &wait]()
    {
        qDebug() << "WARNING : Client() : Bytes Write Timeout";
        written = false;
        wait.quit();
    });
    auto con3 = QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), [&written, &wait]()
    {
        qDebug() << "WARNING : Client() : Socket Error";
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

quint16 SshClient::openLocalPortForwarding(QString servicename, quint16 port, quint16 bind)
{
    if(_channels.contains(servicename))
    {
        return _channels.value(servicename)->localPort();
    }

    SshTunnelIn *tunnel = new SshTunnelIn(this, servicename, port, bind);
    if(!tunnel->valid())
    {
        return 0;
    }
    _channels.insert(servicename, tunnel);
    return tunnel->localPort();
}

quint16 SshClient::openRemotePortForwarding(QString servicename, quint16 port)
{
    if(_channels.contains(servicename))
    {
        return _channels.value(servicename)->localPort();
    }

    SshChannel *tunnel = qobject_cast<SshChannel *>(new SshTunnelOutSrv(this, servicename, port));
    _channels.insert(servicename, tunnel);
    return tunnel->localPort();
}

void SshClient::closePortForwarding(QString servicename)
{
    if(_channels.contains(servicename))
    {
        SshChannel *tunnel = _channels.value(servicename);
        _channels.remove(servicename);
        delete tunnel;
    }
}

QString SshClient::runCommand(QString command)
{
    QString res;
    SshProcess *sshProcess = new SshProcess(this);
    if(sshProcess != nullptr)
    {
        res = sshProcess->runCommand(command);
    }
    return res;
}

QString SshClient::sendFile(QString src, QString dst)
{
    SshScpSend *sender = new SshScpSend(this);
    QString d = sender->send(src, dst);
    delete sender;
    return d;
}

void SshClient::enableSFTP()
{
    if(!_sftp)
    {
        _sftp = new SshSFtp(this);
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : Enable sFtp";
#endif
        QObject::connect(_sftp, &SshSFtp::xfer, this, [this](){
            emit sFtpXfer();
        });
    }
}

QString SshClient::send(QString source, QString dest)
{
    QString res;
#if defined(DEBUG_SFTP)
    qDebug() << "DEBUG : SshClient::sFtpSend(" << source << "," << dest << ")";
#endif
    enableSFTP();
    res = _sftp->send(source, dest);
    return res;
}

bool SshClient::get(QString source, QString dest, bool override)
{
    bool res;
    enableSFTP();
    res = _sftp->get(source, dest, override);
    return res;
}

int SshClient::mkdir(QString dest)
{
    int res;
    enableSFTP();
    res = _sftp->mkdir(dest);
    qDebug() << "DEBUG : SshClient::sFtpMkdir " << dest << " = " << res;
    return res;
}

QStringList SshClient::readdir(QString d)
{
    QStringList res;
    enableSFTP();
    res = _sftp->readdir(d);
    return res;
}

bool SshClient::isDir(QString d)
{
    bool res;
    enableSFTP();
    res = _sftp->isDir(d);
    return res;
}

bool SshClient::isFile(QString d)
{
    bool res;
    enableSFTP();
    res = _sftp->isFile(d);
    return res;
}

int SshClient::mkpath(QString dest)
{
    int res;
    enableSFTP();
    res = _sftp->mkpath(dest);
    return res;
}

bool SshClient::unlink(QString d)
{
    bool res;
    enableSFTP();
    res = _sftp->unlink(d);
    return res;
}

quint64 SshClient::filesize(QString d)
{
    quint64 res;
    enableSFTP();
    res = _sftp->filesize(d);
    return res;
}




int SshClient::connectToHost(const QString & user, const QString & host, quint16 port)
{
    if(_sshConnected) {
        qDebug() << "ERROR : Allways connected";
        return 0;
    }
    QEventLoop wait;
    QTimer timeout;
    _hostname = host;
    _username = user;
    _port = port;
    _errorcode = 0;

#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshClient("<< _name << ") : trying to connect to host (" << _hostname << ":" << _port << ")";
#endif

    _socket.connectToHost(_hostname, _port);
    if(!_socket.waitForConnected(60000))
    {
#if defined(DEBUG_SSHCLIENT)
        qDebug() << "DEBUG : SshClient("<< _name << ") : Failed to connect socket";
#endif
        return -1;
    }

#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshClient("<< _name << ") : ssh socket connected";
#endif

    /* Socket is ready, now start to initialize Ssh Session */
    int sock = static_cast<int>(_socket.socketDescriptor());
    int ret = qssh2_session_handshake(_session, sock);
    if(ret)
    {
        qDebug() << "ERROR : Handshake error " << sshErrorToString(ret);
        _socket.disconnectFromHost();
        _socket.waitForDisconnected(60000);
        return -1;
    }

    /* Ssh session initialized */
    size_t len;
    int type;
    const char * fingerprint = qssh2_session_hostkey(_session, &len, &type);

    if(fingerprint == nullptr)
    {
        qDebug() << "ERROR : Fingerprint error";
        _socket.disconnectFromHost();
        _socket.waitForDisconnected(60000);
        return -1;
    }

    _hostKey.hash = QByteArray(qssh2_hostkey_hash(_session,LIBSSH2_HOSTKEY_HASH_MD5), 16);
    switch (type)
    {
        case LIBSSH2_HOSTKEY_TYPE_RSA:
            _hostKey.type=SshKey::Rsa;
            break;
        case LIBSSH2_HOSTKEY_TYPE_DSS:
            _hostKey.type=SshKey::Dss;
            break;
        default:
            _hostKey.type=SshKey::UnknownType;
    }

    _hostKey.key = QByteArray(fingerprint, static_cast<int>(len));
    struct qssh2_knownhost *khost;
    qssh2_knownhost_check(_knownHosts, _hostname.toStdString().c_str(), fingerprint, len, LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &khost);

    QByteArray username = _username.toLocal8Bit();
    char * alist = nullptr;

    alist = qssh2_userauth_list(_session, username.data(), static_cast<unsigned int>(username.length()));
    if(alist == nullptr)
    {
            int ret = qssh2_session_last_error(_session, nullptr, nullptr, 0);
#if defined(DEBUG_SSHCLIENT)
            qDebug() << "DEBUG : SshClient("<< _name << ") : Failed to authenticate: " << sshErrorToString(ret);
#else
            Q_UNUSED(ret);
#endif
    }

    if (alist == nullptr && !qssh2_userauth_authenticated(_session))
    {
        /* Autentication Error */
        qDebug() << "ERROR : Authentication error " << sshErrorToString(ret);
        _socket.disconnectFromHost();
        _socket.waitForDisconnected(60000);
        return -1;
    }

    QByteArrayList methodes = QByteArray(alist).split(',');

    while(!qssh2_userauth_authenticated(_session) && methodes.length() > 0)
    {
        if(methodes.contains("publickey"))
        {
#if defined(DEBUG_SSHCLIENT)
            qDebug() << "DEBUG : SshClient("<< _name << ") : Trying authentication by publickey";
#endif

            ret = qssh2_userauth_publickey_frommemory(
                            _session,
                            _username.toStdString().c_str(),
                            static_cast<size_t>(_username.length()),
                            _publicKey.toStdString().c_str(),
                            static_cast<size_t>(_publicKey.length()),
                            _privateKey.toStdString().c_str(),
                            static_cast<size_t>(_privateKey.length()),
                            _passphrase.toStdString().c_str()
                    );

            if(ret)
            {
#if defined(DEBUG_SSHCLIENT)
                qDebug() << "DEBUG : SshClient("<< _name << ") : Failed to userauth_publickey: " << sshErrorToString(ret);
#endif
                methodes.removeAll("publickey");
            }
        }
        else if(methodes.contains("password"))
        {
#if defined(DEBUG_SSHCLIENT)
            qDebug() << "DEBUG : SshClient("<< _name << ") : Trying authentication by password";
#endif

            const char *username = _username.toStdString().c_str();
            unsigned int usernameLen = static_cast<unsigned int>(_username.length());
            const char *passphrase = _passphrase.toStdString().c_str();
            unsigned int passphraseLen = static_cast<unsigned int>(_passphrase.length());


            ret = qssh2_userauth_password_ex(_session,
                                                     username,
                                                     usernameLen,
                                                     passphrase,
                                                     passphraseLen);
            if(ret)
            {
#if defined(DEBUG_SSHCLIENT)
                qDebug() << "DEBUG : SshClient("<< _name << ") : Failed to userauth_password: " << sshErrorToString(ret);
#endif
                methodes.removeAll("password");
            }
        }
        else
        {
            qDebug() << "ERROR : Not managed " << methodes.at(0) << " authentication";
            methodes.removeAll(methodes.at(0));
        }
    }

    if(!qssh2_userauth_authenticated(_session))
    {
        /* Autentication Error */
        qDebug() << "ERROR : Authentication error not more available methodes";
        _socket.disconnectFromHost();
        _socket.waitForDisconnected(60000);
        return -1;
    }

#if defined(DEBUG_SSHCLIENT) || defined(DEBUG_THREAD)
    qDebug() << "DEBUG : SshClient("<< _name << ") : Connected and authenticated, @" << this << " in " << QThread::currentThread() << " (" << QThread::currentThreadId() << ")";
#endif

    QObject::connect(&_socket, &QAbstractSocket::readyRead, this, &SshClient::_readyRead);

    _keepalive.setInterval(10000);
    _keepalive.start();
    qssh2_keepalive_config(_session, 1, 5);
    _sshConnected = true;
    return 0;
}

void SshClient::_readyRead()
{
    emit sshDataReceived();
}

void SshClient::disconnectFromHost()
{
    if(!_sshConnected) return;

    QEventLoop wait;

    /* Close all Opened Channels */
    foreach(QString name, _channels.keys()){
        closePortForwarding(name);
    }

    /* Close SFTP if enabled */
    if(!_sftp) {
        delete _sftp;
        _sftp = nullptr;
    }

    _keepalive.stop();
    emit sshReset();

    QObject::disconnect(&_socket, &QAbstractSocket::readyRead, this, &SshClient::_readyRead);

    if (_knownHosts)
    {
        qssh2_knownhost_free(_knownHosts);
    }


    if (_session)
    {
        qssh2_session_free(_session);
    }

    _errorcode = 0;
    _errorMessage = QString();
    _sshConnected = false;
    _session = nullptr;

    disconnect(&_socket, SIGNAL(disconnected()), this, SLOT(_disconnected()));
    if(_socket.state() != QTcpSocket::UnconnectedState)
    {
        _socket.disconnectFromHost();
        if(_socket.state() != QTcpSocket::UnconnectedState)
        {
            _socket.waitForDisconnected();
        }
    }
    _socket.close();
}

void SshClient::setPassphrase(const QString & pass)
{
    _passphrase = pass;
}

void SshClient::setKeys(const QString &publicKey, const QString &privateKey)
{
    _publicKey  = publicKey;
    _privateKey = privateKey;
}

bool SshClient::loadKnownHosts(const QString & file)
{
    bool res = (qssh2_knownhost_readfile(_knownHosts, qPrintable(file), LIBSSH2_KNOWNHOST_FILE_OPENSSH) == 0);
    return (res);
}

bool SshClient::saveKnownHosts(const QString & file)
{
    bool res = (qssh2_knownhost_writefile(_knownHosts, qPrintable(file), LIBSSH2_KNOWNHOST_FILE_OPENSSH) == 0);
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
    ret = (qssh2_knownhost_add(_knownHosts, qPrintable(hostname), NULL, key.key.data(), key.key.size(), typemask, NULL));
    return ret;
}

QString SshClient::banner()
{
    return QString(qssh2_session_banner_get(_session));
}


void SshClient::_tcperror(QAbstractSocket::SocketError err)
{
    if(err == QAbstractSocket::ConnectionRefusedError)
    {
        _errorcode = LIBSSH2_ERROR_BAD_SOCKET;
        qDebug() << "ERROR : "<< _name << " : ConnectionRefusedError";
        emit _connectionTerminate();
    }
    else
    {
        qDebug() << "ERROR : SshClient("<< _name << ") : failed to connect session tcp socket, err=" << err;
    }
}

void SshClient::_cntRate()
{
    emit xfer_rate(_cntTxData, _cntRxData);
    _cntRxData = 0;
    _cntTxData = 0;
}

void SshClient::_sendKeepAlive()
{
    int keepalive;
    qssh2_keepalive_send(_session, &keepalive);
}

void SshClient::askDisconnect()
{
    /* Close all Opened Channels */
    foreach(QString name, _channels.keys()){
        closePortForwarding(name);
    }

#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshClient("<< _name << ") : reset";
#endif
    _keepalive.stop();
    emit sshReset();

    if (_knownHosts)
    {
        qssh2_knownhost_free(_knownHosts);
    }

    if (_session)
    {
        qssh2_session_disconnect(_session, "good bye!");
        qssh2_session_free(_session);
    }
    _errorcode = 0;
    _errorMessage = QString();

    _session = qssh2_session_init_ex(NULL, NULL, NULL,reinterpret_cast<void *>(this));

    qssh2_session_callback_set(_session, LIBSSH2_CALLBACK_RECV,reinterpret_cast<void*>(& qt_callback_libssh_recv));
    qssh2_session_callback_set(_session, LIBSSH2_CALLBACK_SEND,reinterpret_cast<void*>(& qt_callback_libssh_send));
    Q_ASSERT(_session);
    _knownHosts = qssh2_knownhost_init(_session);
    Q_ASSERT(_knownHosts);
    qssh2_session_set_blocking(_session, 0);
    _socket.disconnectFromHost();
#if defined(DEBUG_SSHCLIENT)
    qDebug() << "DEBUG : SshClient("<< _name << ") : reset disconnected";
#endif
}

void SshClient::_disconnected()
{
    _keepalive.stop();

        qWarning() << "WARNING : SshClient("<< _name << ") : unexpected shutdown";
        emit unexpectedDisconnection();
        askDisconnect();

    _sshConnected = false;
    emit disconnected();
}

void SshClient::_getLastError()
{
    char * msg;
    int len = 0;
    _errorcode = qssh2_session_last_error(_session, &msg, &len, 0);
    _errorMessage = QString::fromLocal8Bit(QByteArray::fromRawData(msg, len));
}

