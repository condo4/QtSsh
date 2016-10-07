#ifndef SSHFILESYSTEMNODE_H
#define SSHFILESYSTEMNODE_H

#include <QObject>
#include <QModelIndex>
#include "sshfilesystemmodel.h"
#include "sshfsinterface.h"

class SshFilesystemNode : public QObject
{
    Q_OBJECT
    SshFsInterface *_provider;
    SshFilesystemNode *_parent;
    QList<SshFilesystemNode *> _dirchildren;
    QList<SshFilesystemNode *> _filechildren;
    QString _filename;
    bool _expended;
    mutable QStringList _readdir;
    bool _isdir;
    int _filesize;

public:
    explicit SshFilesystemNode(SshFsInterface *provider, SshFilesystemNode *parent, QString path);

    SshFilesystemNode *child(int row);
    SshFilesystemNode *parent() const;
    QString path() const;

    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int childId(const SshFilesystemNode *) const;
    int row() const;



    bool isdir() const;

public slots:

private:
    void _expend();
};

#endif // SSHFILESYSTEMNODE_H
