#include "sshworker.h"
#include "sshclient.h"
#include <QEventLoop>

SshWorker::SshWorker(QObject *parent) : QThread(parent)
{
    QEventLoop wait;
    QObject::connect(this, &SshWorker::threadReady, &wait, &QEventLoop::quit);
    start();
    wait.exec();
}

SshWorker::~SshWorker()
{
    QEventLoop wait;
    QObject::connect(this, &SshWorker::finished, &wait, &QEventLoop::quit);
    emit askQuit();
    wait.exec();
}

void SshWorker::setKeys(const QString &publicKey, const QString &privateKey)
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::setKeysTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "setKeys", Qt::QueuedConnection, Q_ARG( const QString, publicKey ), Q_ARG( const QString, privateKey ) );
    wait.exec();
    QObject::disconnect(con);
}

bool SshWorker::loadKnownHosts(const QString &file)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::loadKnownHostsTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "loadKnownHosts", Qt::QueuedConnection, Q_ARG( const QString, file ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

int SshWorker::connectToHost(const QString &username, const QString &hostname, quint16 port, bool lock, bool checkHostKey, unsigned int retry)
{
    QEventLoop wait;
    int ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::connectSshToHostTerminate, this, [&wait, &ret](int res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "connectToHost", Qt::QueuedConnection, Q_ARG(QString, username), Q_ARG(QString, hostname), Q_ARG(quint16, port), Q_ARG(bool, lock), Q_ARG(bool, checkHostKey), Q_ARG(unsigned int, retry) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::disconnectFromHost()
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::disconnectSshFromHostTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "disconnectFromHost", Qt::QueuedConnection);
    wait.exec();
    QObject::disconnect(con);
}

QString SshWorker::runCommand(QString command)
{
    QEventLoop wait;
    QString ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::runCommandTerminate, this, [&wait, &ret](QString res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "runCommand", Qt::QueuedConnection, Q_ARG( QString, command ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

quint16 SshWorker::openLocalPortForwarding(QString servicename, quint16 port, quint16 bind)
{
    QEventLoop wait;
    quint16 ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::openLocalPortForwardingTerminate, this, [&wait, &ret](quint16 res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "openLocalPortForwarding", Qt::QueuedConnection, Q_ARG( QString, servicename ), Q_ARG( quint16, port ), Q_ARG( quint16, bind ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

quint16 SshWorker::openRemotePortForwarding(QString servicename, quint16 port)
{
    QEventLoop wait;
    quint16 ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::openRemotePortForwardingTerminate, this, [&wait, &ret](quint16 res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "openRemotePortForwarding", Qt::QueuedConnection, Q_ARG( QString, servicename ), Q_ARG( quint16, port ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::closePortForwarding(QString servicename)
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::closePortForwardingTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "closePortForwarding", Qt::QueuedConnection, Q_ARG( QString, servicename ) );
    wait.exec();
    QObject::disconnect(con);
}

QString SshWorker::sendFile(QString src, QString dst)
{
    QEventLoop wait;
    QString ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sendFileTerminate, this, [&wait, &ret](QString res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "sendFile", Qt::QueuedConnection, Q_ARG( QString, src ), Q_ARG( QString, dst ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::setPassphrase(const QString &pass)
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::setPassphraseTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "setPassphrase", Qt::QueuedConnection, Q_ARG( QString, pass ) );
    wait.exec();
    QObject::disconnect(con);
}

bool SshWorker::saveKnownHosts(const QString &file)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::saveKnownHostsTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "saveKnownHosts", Qt::QueuedConnection, Q_ARG( QString, file ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::addKnownHost(const QString &hostname, const SshKey &key)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::addKnownHostTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "addKnownHost", Qt::QueuedConnection, Q_ARG( QString, hostname ), Q_ARG( SshKey, key ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::enableSFTP()
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::enableSftpTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "enableSFTP", Qt::QueuedConnection );
    wait.exec();
    QObject::disconnect(con);
}

QString SshWorker::send(QString source, QString dest)
{
    QEventLoop wait;
    QString ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpSendTerminate, this, [&wait, &ret](QString res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "send", Qt::QueuedConnection, Q_ARG( QString, source ), Q_ARG( QString, dest ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::get(QString source, QString dest, bool override)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpGetTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "send", Qt::QueuedConnection, Q_ARG( QString, source ), Q_ARG( QString, dest ), Q_ARG( bool, override ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

int SshWorker::mkdir(QString dest)
{
    QEventLoop wait;
    int ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpMkdirTerminate, this, [&wait, &ret](int res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "mkdir", Qt::QueuedConnection,  Q_ARG( QString, dest ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

QStringList SshWorker::readdir(QString d)
{
    QEventLoop wait;
    QStringList ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpDirTerminate, this, [&wait, &ret](QStringList res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "readdir", Qt::QueuedConnection,  Q_ARG( QString, d ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::isDir(QString d)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpIsDirTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "isDir", Qt::QueuedConnection,  Q_ARG( QString, d ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::isFile(QString d)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpIsFileTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "isFile", Qt::QueuedConnection,  Q_ARG( QString, d ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

int SshWorker::mkpath(QString dest)
{
    QEventLoop wait;
    int ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpMkpathTerminate, this, [&wait, &ret](int res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "mkpath", Qt::QueuedConnection,  Q_ARG( QString, dest ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::unlink(QString d)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpUnlinkTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "unlink", Qt::QueuedConnection,  Q_ARG( QString, d ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

quint64 SshWorker::filesize(QString d)
{
    QEventLoop wait;
    quint64 ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpFileSizeTerminate, this, [&wait, &ret](quint64 res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    QMetaObject::invokeMethod( _client, "filesize", Qt::QueuedConnection,  Q_ARG( QString, d ) );
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::xferRate(qint64 tx, qint64 rx)
{
    emit xfer_rate(tx, rx);
}

void SshWorker::run()
{
    QEventLoop loop;
    connect(this, &SshWorker::askQuit, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    _client = new SshClient();
    QObject::connect(_client, &SshClient::xfer_rate,                   this,    &SshWorker::xferRate);
    QObject::connect(_client, &SshClient::sFtpXfer,                    this,    &SshWorker::sFtpXfer);

    emit threadReady();
    loop.exec();
#if defined(DEBUG_SSHWORKER)
    qDebug() << "DEBUG : SshWorker thread terminated " << " in Thread " << QThread::currentThread();
#endif
}
