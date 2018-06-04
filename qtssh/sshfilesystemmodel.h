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
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3
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
