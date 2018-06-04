#include "sshfilesystemnode.h"
#include <qdebug.h>
#include <QTime>
#include "sshsftp.h"


bool SshFilesystemNode::isdir() const
{
    return m_isdir;
}

SshFilesystemNode::SshFilesystemNode(SshSFtp *provider, SshFilesystemNode *parent, QString path):
    QObject(parent),
    m_provider(provider),
    m_parent(parent),
    m_filename(path),
    m_expended(false)
{
#if defined(DEBUG_SSHFILESYSTEMNODE)
    qDebug() << "Create FileSystemNode " << this << " with path = " << this->path();
#endif
    m_isdir = m_provider->isDir(this->path());
    if(m_isdir)
    {
        m_readdir.append(m_provider->readdir(this->path()));
        m_readdir.removeAll(".");
        m_readdir.removeAll("..");
        m_filesize = 0;
    }
    else
    {
        m_filesize = m_provider->filesize(this->path());
    }
}

SshFilesystemNode *SshFilesystemNode::child(int row)
{
    if(!m_expended) _expend();
    if(row >= (m_dirchildren.count() + m_filechildren.count()) ) return nullptr;

    if(row < m_dirchildren.count())
    {
        return m_dirchildren[row];
    }
    else
    {
        return m_filechildren[row - m_dirchildren.count()];
    }


}

SshFilesystemNode *SshFilesystemNode::parent() const
{
    return m_parent;
}

QString SshFilesystemNode::path() const
{
    if(m_parent) return m_parent->path() + "/" + m_filename;
    else return (m_filename.isEmpty())?("/"):(m_filename);
}

int SshFilesystemNode::childCount() const
{
    if(!m_isdir) return 0;
    return m_readdir.count();
}

int SshFilesystemNode::columnCount() const
{
    return 2;
}

QVariant SshFilesystemNode::data(int column) const
{
    if(column == 0) return m_filename;
    if(column == 1) {
        if(isdir()) return childCount();
        else return m_filesize;
    }
    return QVariant();
}

int SshFilesystemNode::childId(const SshFilesystemNode *node) const
{
    if(node->isdir()) return m_dirchildren.indexOf((SshFilesystemNode*)node);
    else return m_filechildren.indexOf((SshFilesystemNode*)node) + m_dirchildren.count();
}

int SshFilesystemNode::row() const
{
    if(!m_parent) return 0;
    else return m_parent->childId(this);
}

void SshFilesystemNode::_expend()
{
    if(m_expended) return;
    if(!m_isdir) return;
#if DEBUG_SSHFILESYSTEMNODE
    qDebug() <<  "START EXPEND " + _filename;
#endif
    foreach(QString item, m_readdir)
    {
        SshFilesystemNode* child = new SshFilesystemNode(m_provider, this, item);
#if DEBUG_SSHFILESYSTEMNODE
        qDebug() << "Created child " << child->path() << " is " << (child->isdir()?"dir":"file");
#endif
        if(child->isdir()) m_dirchildren.append(child);
        else m_filechildren.append(child);
    }
    m_expended = true;
#if DEBUG_SSHFILESYSTEMNODE
    qDebug() <<  "END EXPEND " + _filename;
#endif
}
