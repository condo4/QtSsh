#include "sshfilesystemmodel.h"
#include "sshfilesystemnode.h"
#include <qdebug.h>
#include "sshsftp.h"
#include "sshfilesystemnode.h"

SshFilesystemModel::SshFilesystemModel(SshSFtp *provider)
    :  m_provider(provider)
{
    m_rootItem = new SshFilesystemNode(m_provider, nullptr, "/");
    m_roles.insert(0, "fileName");
    m_roles.insert(1, "fileSize");
}

QModelIndex SshFilesystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    SshFilesystemNode *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<SshFilesystemNode*>(parent.internalPointer());

    SshFilesystemNode *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex SshFilesystemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    SshFilesystemNode *childItem = static_cast<SshFilesystemNode*>(index.internalPointer());
    SshFilesystemNode *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SshFilesystemModel::rowCount(const QModelIndex &parent) const
{
    SshFilesystemNode *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<SshFilesystemNode*>(parent.internalPointer());

    return parentItem->childCount();
}

int SshFilesystemModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<SshFilesystemNode*>(parent.internalPointer())->columnCount();

    return m_rootItem->columnCount();
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
    return m_roles;
}
