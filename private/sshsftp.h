#ifndef SSHSFTP_H
#define SSHSFTP_H

#include "sshchannel.h"
#include <libssh2_sftp.h>
#include <QEventLoop>
#include <QTimer>
#include <QStringList>
#include <QHash>
#include "sshfsinterface.h"

class SshSFtp : public SshChannel, public SshFsInterface
{
    Q_OBJECT

private:
    LIBSSH2_SFTP *_sftpSession;
    QString _mkdir;

    bool _waitData(int timeout);
    QHash<QString,  LIBSSH2_SFTP_HANDLE *> _dirhandler;
    QHash<QString,  LIBSSH2_SFTP_ATTRIBUTES> _fileinfo;

    LIBSSH2_SFTP_HANDLE *getDirHandler(QString path);
    LIBSSH2_SFTP_ATTRIBUTES getFileInfo(QString path);


public:
    SshSFtp(SshClient * client);
    ~SshSFtp();


    /* <<<SshFsInterface>>> */
    void enableSFTP();
    QString send(QString source, QString dest);
    bool get(QString source, QString dest, bool override = false);
    int mkdir(QString dest);
    QStringList readdir(QString d);
    bool isDir(QString d);
    bool isFile(QString d);
    int mkpath(QString dest);
    bool unlink(QString d);
    quint64 filesize(QString d);
    /* >>>SshFsInterface<<< */

protected slots:
    void sshDataReceived();

signals:
    void sshData();
    void xfer();
};

#endif // SSHSFTP_H
