#pragma once

#include <QObject>
#include <QModelIndex>
#include "sshfilesystemmodel.h"

class SshSFtp;

class SshFilesystemNode : public QObject
{
    Q_OBJECT
    SshSFtp *m_provider;
    SshFilesystemNode *m_parent;
    QList<SshFilesystemNode *> m_dirchildren;
    QList<SshFilesystemNode *> m_filechildren;
    QString m_filename;
    bool m_expended;
    mutable QStringList m_readdir;
    bool m_isdir;
    int m_filesize;

public:
    explicit SshFilesystemNode(SshSFtp *provider, SshFilesystemNode *parent, QString path);

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
