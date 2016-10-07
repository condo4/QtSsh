#include "sshfilesystemmodel.h"
#include "sshfilesystemnode.h"
#include "qdebug.h"

SshFilesystemModel::SshFilesystemModel(SshFsInterface *provider):
    QAbstractItemModel(),
    _provider(provider)
{
    _rootItem = new SshFilesystemNode(_provider, nullptr, "/");
    _roles.insert(0, "fileName");
    _roles.insert(1, "fileSize");
}

QModelIndex SshFilesystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    SshFilesystemNode *parentItem;

    if (!parent.isValid())
        parentItem = _rootItem;
    else
        parentItem = static_cast<SshFilesystemNode*>(parent.internalPointer());

    SshFilesystemNode *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex SshFilesystemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    SshFilesystemNode *childItem = static_cast<SshFilesystemNode*>(index.internalPointer());
    SshFilesystemNode *parentItem = childItem->parent();

    if (parentItem == _rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SshFilesystemModel::rowCount(const QModelIndex &parent) const
{
    SshFilesystemNode *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = _rootItem;
    else
        parentItem = static_cast<SshFilesystemNode*>(parent.internalPointer());

    return parentItem->childCount();
}

int SshFilesystemModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<SshFilesystemNode*>(parent.internalPointer())->columnCount();
    else
        return _rootItem->columnCount();
}

QVariant SshFilesystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SshFilesystemNode *item = static_cast<SshFilesystemNode*>(index.internalPointer());

    return item->data(role);
}

QHash<int, QByteArray> SshFilesystemModel::roleNames() const
{
    return _roles;
}
