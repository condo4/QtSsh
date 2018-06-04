#pragma once

#include "sshchannel.h"
#include <libssh2_sftp.h>
#include <QEventLoop>
#include <QTimer>
#include <QStringList>
#include <QHash>

class SshSFtp : public SshChannel
{
    Q_OBJECT

private:
    LIBSSH2_SFTP *m_sftpSession;
    QString m_mkdir;

    bool m_waitData(int timeout);
    QHash<QString,  LIBSSH2_SFTP_HANDLE *> m_dirhandler;
    QHash<QString,  LIBSSH2_SFTP_ATTRIBUTES> m_fileinfo;

    LIBSSH2_SFTP_HANDLE *getDirHandler(QString path);
    LIBSSH2_SFTP_HANDLE *closeDirHandler(QString path);
    LIBSSH2_SFTP_ATTRIBUTES getFileInfo(QString path);


public:
    SshSFtp(SshClient * client);
    ~SshSFtp();

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


protected slots:
    void sshDataReceived();

signals:
    void sshData();
    void xfer();
};
