#ifndef SSHFILESYSTEMMODEL_H
#define SSHFILESYSTEMMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QHash>
#include "sshfsinterface.h"

class SshFilesystemNode;

class SshFilesystemModel : public QAbstractItemModel
{
    SshFsInterface *_provider;
    SshFilesystemNode *_rootItem;
    QModelIndex _root;
    QHash<int, QByteArray> _roles;

public:
    enum Roles {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3
    };

    SshFilesystemModel(SshFsInterface *provider);

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &index = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    void setRootPath(QString root = "/");
};

#endif // SSHFILESYSTEMMODEL_H
