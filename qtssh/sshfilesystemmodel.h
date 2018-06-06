#pragma once

#include <QObject>
#include <QAbstractItemModel>
#include <QHash>

class SshSFtp;
class SshFilesystemNode;

class SshFilesystemModel : public QAbstractItemModel
{
    SshSFtp *m_provider;
    SshFilesystemNode *m_rootItem;
    QModelIndex m_root;
    QHash<int, QByteArray> m_roles;

public:
    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole = Qt::UserRole + 2,
        FileBaseNameRole = Qt::UserRole + 3,
        FileSuffixRole = Qt::UserRole + 4,
        FileSizeRole = Qt::UserRole + 5,
        FileLastModifiedRole = Qt::UserRole + 6,
        FileLastReadRole = Qt::UserRole +7,
        FileIsDirRole = Qt::UserRole + 8,
        FileUrlRole = Qt::UserRole + 9
    };

    SshFilesystemModel(SshSFtp *provider);

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &index = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    void setRootPath(QString root = "/");
};
