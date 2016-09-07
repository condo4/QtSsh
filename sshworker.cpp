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
    emit askSetKeys(publicKey, privateKey);
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
    emit askLoadKnownHosts(file);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

int SshWorker::connectSshToHost(const QString &username, const QString &hostname, quint16 port, bool lock, bool checkHostKey, unsigned int retry)
{
    QEventLoop wait;
    int ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::connectSshToHostTerminate, this, [&wait, &ret](int res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askConnectSshToHost(username, hostname, port, lock, checkHostKey, retry);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::disconnectSshFromHost()
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::disconnectSshFromHostTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    emit askDisconnectSshFromHost();
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
    emit askRunCommand(command);
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
    emit askOpenLocalPortForwarding(servicename, port, bind);
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
    emit askOpenRemotePortForwarding(servicename, port);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::closePortForwarding(QString servicename)
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::closePortForwardingTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    emit askClosePortForwarding(servicename);
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
    emit askSendFile(src, dst);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

void SshWorker::enableSftp()
{
    QEventLoop wait;
    QMetaObject::Connection con = connect(_client, &SshClient::enableSftpTerminate, &wait, &QEventLoop::quit, Qt::QueuedConnection);
    emit askEnableSftp();
    wait.exec();
    QObject::disconnect(con);
}

QString SshWorker::sFtpSend(QString source, QString dest)
{
    QEventLoop wait;
    QString ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpSendTerminate, this, [&wait, &ret](QString res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpSend(source, dest);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::sFtpGet(QString source, QString dest, bool override)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpGetTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpGet(source, dest, override);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

int SshWorker::sFtpMkdir(QString dest)
{
    QEventLoop wait;
    int ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpMkdirTerminate, this, [&wait, &ret](int res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpMkdir(dest);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

QStringList SshWorker::sFtpDir(QString d)
{
    QEventLoop wait;
    QStringList ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpDirTerminate, this, [&wait, &ret](QStringList res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpDir(d);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::sFtpIsDir(QString d)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpIsDirTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpIsDir(d);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::sFtpIsFile(QString d)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpIsFileTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpIsFile(d);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

int SshWorker::sFtpMkpath(QString dest)
{
    QEventLoop wait;
    int ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpMkpathTerminate, this, [&wait, &ret](int res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpMkpath(dest);
    wait.exec();
    QObject::disconnect(con);
    return ret;
}

bool SshWorker::sFtpUnlink(QString d)
{
    QEventLoop wait;
    bool ret;
    QMetaObject::Connection con = QObject::connect(_client, &SshClient::sFtpUnlinkTerminate, this, [&wait, &ret](bool res) {
        ret = res;
        wait.quit();
    },Qt::QueuedConnection);
    emit askSFtpUnlink(d);
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
    QObject::connect(this,    &SshWorker::askSetKeys,                  _client, &SshClient::setKeys);
    QObject::connect(this,    &SshWorker::askLoadKnownHosts,           _client, &SshClient::loadKnownHosts);
    QObject::connect(this,    &SshWorker::askConnectSshToHost,         _client, &SshClient::connectSshToHost);
    QObject::connect(this,    &SshWorker::askDisconnectSshFromHost,    _client, &SshClient::disconnectSshFromHost);
    QObject::connect(this,    &SshWorker::askRunCommand,               _client, &SshClient::runCommand);
    QObject::connect(this,    &SshWorker::askOpenLocalPortForwarding,  _client, &SshClient::openLocalPortForwarding);
    QObject::connect(this,    &SshWorker::askOpenRemotePortForwarding, _client, &SshClient::openRemotePortForwarding);
    QObject::connect(this,    &SshWorker::askClosePortForwarding,      _client, &SshClient::closePortForwarding);
    QObject::connect(this,    &SshWorker::askSendFile,                 _client, &SshClient::sendFile);

    QObject::connect(this,    &SshWorker::askEnableSftp,               _client, &SshClient::enableSftp);
    QObject::connect(this,    &SshWorker::askSFtpSend,                 _client, &SshClient::sFtpSend);
    QObject::connect(this,    &SshWorker::askSFtpGet,                  _client, &SshClient::sFtpGet);
    QObject::connect(this,    &SshWorker::askSFtpMkdir,                _client, &SshClient::sFtpMkdir);
    QObject::connect(this,    &SshWorker::askSFtpDir,                  _client, &SshClient::sFtpDir);
    QObject::connect(this,    &SshWorker::askSFtpIsDir,                _client, &SshClient::sFtpIsDir);
    QObject::connect(this,    &SshWorker::askSFtpIsFile,               _client, &SshClient::sFtpIsFile);
    QObject::connect(this,    &SshWorker::askSFtpMkpath,               _client, &SshClient::sFtpMkpath);
    QObject::connect(this,    &SshWorker::askSFtpUnlink,               _client, &SshClient::sFtpUnlink);

    QObject::connect(_client, &SshClient::xfer_rate,                   this,    &SshWorker::xferRate);
    QObject::connect(_client, &SshClient::sFtpXfer,                    this,    &SshWorker::sFtpXfer);

    emit threadReady();
    loop.exec();
#if defined(DEBUG_SSHWORKER)
    qDebug() << "DEBUG : SshWorker thread terminated " << " in Thread " << QThread::currentThread();
#endif
}
