#ifndef SSHFS_H
#define SSHFS_H

class SshFsInterface
{
public slots:
    virtual ~SshFsInterface() {}
    virtual void enableSFTP() = 0;
    virtual QString send(QString source, QString dest) = 0;
    virtual bool get(QString source, QString dest, bool override = false) = 0;
    virtual int mkdir(QString dest) = 0;
    virtual QStringList readdir(QString d) = 0;
    virtual bool isDir(QString d) = 0;
    virtual bool isFile(QString d) = 0;
    virtual int mkpath(QString dest) = 0;
    virtual bool unlink(QString d) = 0;
    virtual quint64 filesize(QString d) = 0;
};

#endif // SSHFS_H
