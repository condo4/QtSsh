#include "sshprocess.h"
#include "sshclient.h"
#include <QTimer>
#include <QEventLoop>

SshProcess::SshProcess(SshClient *client) : SshChannel(client)
{
    sshChannel = qssh2_channel_open(sshClient->session());
    if (sshChannel == nullptr)
    {
#if defined(DEBUG_SSHCHANNEL)
        qDebug() << "DEBUG : QtSshChannel : channel session open failed";
#endif
        return;
    }
    QObject::connect(client, &SshClient::sshDataReceived, this, &SshProcess::sshDataReceived, Qt::QueuedConnection);

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : QtSshChannel : channel session opened";
#endif
}

SshProcess::~SshProcess()
{
    QObject::disconnect(sshClient, &SshClient::sshDataReceived, this, &SshProcess::sshDataReceived);
    stopChannel();
}

QByteArray SshProcess::runCommand(const QString &cmd)
{
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : QtSshChannel : runCommand(" << cmd << ")";
#endif
    int ret = qssh2_channel_exec(sshChannel, cmd.toStdString().c_str());
    if (ret)
    {
        qDebug() << "ERROR : QtSshChannel : process exec failed " << ret;
        return QByteArray();
    }

    bool eof = false;
    QByteArray result;
    ssize_t retsz;
    char buffer[16*1024];

    while(!eof)
    {

        retsz = qssh2_channel_read(sshChannel, buffer, 16 * 1024);
        if(retsz < 0)
        {
            qDebug() << "ERROR: can't read result (" << retsz << ")";
            return QByteArray();
        }
        result.append(buffer, static_cast<int>(retsz));

        if (qssh2_channel_eof(sshChannel) == 1)
        {
            eof = true;
        }
    }
#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : QtSshChannel : runCommand(" << cmd << ") -> " << result;
#endif
    return result;
}
