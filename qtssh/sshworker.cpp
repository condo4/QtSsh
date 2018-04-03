#include "sshworker.h"
#include "sshclient.h"
#include <QEventLoop>

SshWorker::SshWorker(QString name, QObject *parent, bool detached):
    QThread(parent),
    _name(name)
{
#if defined(DISABLE_MULTITHREAD_SSH_WORKER)
    detached = false;
#endif
    _prepared = false;
    if(detached)
    {
        _contype = Qt::BlockingQueuedConnection;
        QEventLoop wait;
        QObject::connect(this, &SshWorker::threadReady, &wait, &QEventLoop::quit);
        start();
        wait.exec();
    }
    else
    {
        _contype = Qt::DirectConnection;
        _client = new SshClient(_name);
        QObject::connect(_client, &SshClient::xfer_rate,                   this,    &SshWorker::xferRate);
        QObject::connect(_client, &SshClient::sFtpXfer,                    this,    &SshWorker::sFtpXfer);
        QObject::connect(_client, &SshClient::unexpectedDisconnection,     this,    [this](){
            emit unexpectedDisconnection();
        });
        emit threadReady();
    }
}

SshWorker::~SshWorker()
{
    if(_contype == Qt::BlockingQueuedConnection)
    {
        QEventLoop wait;
        QObject::connect(this, &SshWorker::finished, &wait, &QEventLoop::quit);
        emit askQuit();
        wait.exec();
    }
}

bool SshWorker::getSshConnected() const
{
    bool ret;
    QMetaObject::invokeMethod( _client, "getSshConnected", _contype, Q_RETURN_ARG(bool, ret));
    return ret;
}

void SshWorker::setKeys(const QString &publicKey, const QString &privateKey)
{
    QMetaObject::invokeMethod( _client, "setKeys", _contype, Q_ARG( const QString, publicKey ), Q_ARG( const QString, privateKey ) );
}

bool SshWorker::loadKnownHosts(const QString &file)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "loadKnownHosts", _contype, Q_RETURN_ARG(bool, ret), Q_ARG( const QString, file ) );
    return ret;
}

int SshWorker::connectToHost(const QString &username, const QString &hostname, quint16 port, bool lock, bool checkHostKey, unsigned int retry)
{
    int ret;
    QMetaObject::invokeMethod( _client, "connectToHost", _contype, Q_RETURN_ARG(int, ret), Q_ARG(QString, username), Q_ARG(QString, hostname), Q_ARG(quint16, port), Q_ARG(bool, lock), Q_ARG(bool, checkHostKey), Q_ARG(unsigned int, retry) );
    return ret;
}

void SshWorker::prepareConnectToHost(const QString &username, const QString &hostname, quint16 port, bool lock, bool checkHostKey, unsigned int retry)
{
    _username = username;
    _hostname = hostname;
    _port = port;
    _lock = lock;
    _checkHostKey = checkHostKey;
    _retry = retry;
    _prepared = true;
}

int SshWorker::connectToHost()
{
    if(_prepared) return connectToHost(_username, _hostname, _port, _lock, _checkHostKey, _retry);
    else return -1;
}

void SshWorker::disconnectFromHost()
{
    QMetaObject::invokeMethod( _client, "disconnectFromHost", _contype);
}

QString SshWorker::runCommand(QString command)
{
    QString ret;
    QMetaObject::invokeMethod( _client, "runCommand", _contype, Q_RETURN_ARG(QString, ret), Q_ARG( QString, command ) );
    return ret;
}

quint16 SshWorker::openLocalPortForwarding(QString servicename, quint16 port, quint16 bind)
{
    quint16 ret;
    QMetaObject::invokeMethod( _client, "openLocalPortForwarding", _contype, Q_RETURN_ARG(quint16, ret), Q_ARG( QString, servicename ), Q_ARG( quint16, port ), Q_ARG( quint16, bind ) );
    return ret;
}

quint16 SshWorker::openRemotePortForwarding(QString servicename, quint16 port)
{
    quint16 ret;
    QMetaObject::invokeMethod( _client, "openRemotePortForwarding", _contype, Q_RETURN_ARG(quint16, ret), Q_ARG( QString, servicename ), Q_ARG( quint16, port ) );
    return ret;
}

void SshWorker::closePortForwarding(QString servicename)
{
    QMetaObject::invokeMethod( _client, "closePortForwarding", _contype, Q_ARG( QString, servicename ) );
}

QString SshWorker::sendFile(QString src, QString dst)
{
    QString ret;
    QMetaObject::invokeMethod( _client, "sendFile", _contype, Q_RETURN_ARG(QString, ret), Q_ARG( QString, src ), Q_ARG( QString, dst ) );
    return ret;
}

void SshWorker::setPassphrase(const QString &pass)
{
    QMetaObject::invokeMethod( _client, "setPassphrase", _contype, Q_ARG( QString, pass ) );
}

bool SshWorker::saveKnownHosts(const QString &file)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "saveKnownHosts", _contype, Q_RETURN_ARG(bool, ret), Q_ARG( QString, file ) );
    return ret;
}

bool SshWorker::addKnownHost(const QString &hostname, const SshKey &key)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "addKnownHost", _contype, Q_RETURN_ARG(bool, ret), Q_ARG( QString, hostname ), Q_ARG( SshKey, key ) );
    return ret;
}

void SshWorker::enableSFTP()
{
    QMetaObject::invokeMethod( _client, "enableSFTP", _contype );
}

QString SshWorker::send(QString source, QString dest)
{
    QString ret;
    QMetaObject::invokeMethod( _client, "send", _contype, Q_RETURN_ARG(QString, ret), Q_ARG( QString, source ), Q_ARG( QString, dest ) );
    return ret;
}

bool SshWorker::get(QString source, QString dest, bool override)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "get", _contype, Q_RETURN_ARG(bool, ret), Q_ARG( QString, source ), Q_ARG( QString, dest ), Q_ARG( bool, override ) );
    return ret;
}

int SshWorker::mkdir(QString dest)
{
    int ret;
    QMetaObject::invokeMethod( _client, "mkdir", _contype,  Q_RETURN_ARG(int, ret), Q_ARG( QString, dest ) );
    return ret;
}

QStringList SshWorker::readdir(QString d)
{
    QStringList ret;
    QMetaObject::invokeMethod( _client, "readdir", _contype, Q_RETURN_ARG(QStringList, ret), Q_ARG( QString, d ) );
    return ret;
}

bool SshWorker::isDir(QString d)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "isDir", _contype,  Q_RETURN_ARG(bool, ret), Q_ARG( QString, d ) );
    return ret;
}

bool SshWorker::isFile(QString d)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "isFile", _contype, Q_RETURN_ARG(bool, ret), Q_ARG( QString, d ) );
    return ret;
}

int SshWorker::mkpath(QString dest)
{
    int ret;
    QMetaObject::invokeMethod( _client, "mkpath", _contype, Q_RETURN_ARG(int, ret), Q_ARG( QString, dest ) );
    return ret;
}

bool SshWorker::unlink(QString d)
{
    bool ret;
    QMetaObject::invokeMethod( _client, "unlink", _contype, Q_RETURN_ARG(bool, ret), Q_ARG( QString, d ) );
    return ret;
}

quint64 SshWorker::filesize(QString d)
{
    quint64 ret;
    QMetaObject::invokeMethod( _client, "filesize", Qt::QueuedConnection, Q_RETURN_ARG(quint64, ret), Q_ARG( QString, d ) );
    return ret;
}

void SshWorker::xferRate(qint64 tx, qint64 rx)
{
    emit xfer_rate(tx, rx);
}

void SshWorker::run()
{
    connect(this, &SshWorker::askQuit, this, &SshWorker::quit, Qt::QueuedConnection);
    _client = new SshClient("thread_" + _name);
    QObject::connect(_client, &SshClient::xfer_rate,                   this,    &SshWorker::xferRate);
    QObject::connect(_client, &SshClient::sFtpXfer,                    this,    &SshWorker::sFtpXfer);
    QObject::connect(_client, &SshClient::unexpectedDisconnection,     this,    [this](){
        emit unexpectedDisconnection();
    });

    emit threadReady();
    exec();

#if defined(DEBUG_SSHWORKER)
    qDebug() << "DEBUG : SshWorker thread terminated " << " in Thread " << QThread::currentThread();
#endif
}
