#pragma once

#include "sshchannel.h"
#include <libssh2_sftp.h>
#include <QEventLoop>
#include <QTimer>
#include <QStringList>
#include <QHash>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logsshsftp)

class SshSFtp : public SshChannel
{
    Q_OBJECT

private:
    LIBSSH2_SFTP *m_sftpSession {nullptr};
    QString m_mkdir;

    QHash<QString,  LIBSSH2_SFTP_ATTRIBUTES> m_fileinfo;
    LIBSSH2_SFTP_ATTRIBUTES getFileInfo(const QString &path);

protected:
    SshSFtp(const QString &name, SshClient * client);
    friend class SshClient;

public:
    virtual ~SshSFtp() override;

    void close() override;
    void free();

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
    void sshDataReceived() override;

private:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};

};
