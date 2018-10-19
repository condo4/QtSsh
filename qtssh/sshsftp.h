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

    LIBSSH2_SFTP_HANDLE *getDirHandler(const QString &path);
    LIBSSH2_SFTP_HANDLE *closeDirHandler(const QString &path);
    LIBSSH2_SFTP_ATTRIBUTES getFileInfo(const QString &path);


public:
    SshSFtp(SshClient * client);
    ~SshSFtp();

    void enableSFTP();
    QString send(const QString &source, QString dest);
    bool get(const QString &source, QString dest, bool override = false);
    int mkdir(const QString &dest);
    QStringList readdir(const QString &d);
    bool isDir(const QString &d);
    bool isFile(const QString &d);
    int mkpath(const QString &dest);
    bool unlink(const QString &d);
    quint64 filesize(const QString &d);


protected slots:
    void sshDataReceived();

signals:
    void sshData();
    void xfer();
};
