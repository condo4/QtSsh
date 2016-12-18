#include "sshfilesystemnode.h"
#include "qdebug.h"
#include <QTime>


bool SshFilesystemNode::isdir() const
{
    return _isdir;
}

SshFilesystemNode::SshFilesystemNode(SshFsInterface *provider, SshFilesystemNode *parent, QString path):
    QObject(parent),
    _provider(provider),
    _parent(parent),
    _filename(path),
    _expended(false)
{
#if defined(DEBUG_SSHFILESYSTEMNODE)
    qDebug() << "Create FileSystemNode " << this << " with path = " << this->path();
#endif
    _isdir = _provider->isDir(this->path());
    if(_isdir)
    {
        _readdir.append(_provider->readdir(this->path()));
        _readdir.removeAll(".");
        _readdir.removeAll("..");
        _filesize = 0;
    }
    else
    {
        _filesize = _provider->filesize(this->path());
    }
}

SshFilesystemNode *SshFilesystemNode::child(int row)
{
    if(!_expended) _expend();
    if(row >= (_dirchildren.count() + _filechildren.count()) ) return nullptr;

    if(row < _dirchildren.count())
    {
        return _dirchildren[row];
    }
    else
    {
        return _filechildren[row - _dirchildren.count()];
    }


}

SshFilesystemNode *SshFilesystemNode::parent() const
{
    return _parent;
}

QString SshFilesystemNode::path() const
{
    if(_parent) return _parent->path() + "/" + _filename;
    else return (_filename.isEmpty())?("/"):(_filename);
}

int SshFilesystemNode::childCount() const
{
    if(!_isdir) return 0;
    return _readdir.count();
}

int SshFilesystemNode::columnCount() const
{
    return 2;
}

QVariant SshFilesystemNode::data(int column) const
{
    if(column == 0) return _filename;
    if(column == 1) {
        if(isdir()) return childCount();
        else return _filesize;
    }
    return QVariant();
}

int SshFilesystemNode::childId(const SshFilesystemNode *node) const
{
    if(node->isdir()) return _dirchildren.indexOf((SshFilesystemNode*)node);
    else return _filechildren.indexOf((SshFilesystemNode*)node) + _dirchildren.count();
}

int SshFilesystemNode::row() const
{
    if(!_parent) return 0;
    else return _parent->childId(this);
}

void SshFilesystemNode::_expend()
{
    if(_expended) return;
    if(!_isdir) return;
#if DEBUG_SSHFILESYSTEMNODE
    qDebug() <<  "START EXPEND " + _filename;
#endif
    foreach(QString item, _readdir)
    {
        SshFilesystemNode* child = new SshFilesystemNode(_provider, this, item);
#if DEBUG_SSHFILESYSTEMNODE
        qDebug() << "Created child " << child->path() << " is " << (child->isdir()?"dir":"file");
#endif
        if(child->isdir()) _dirchildren.append(child);
        else _filechildren.append(child);
    }
    _expended = true;
#if DEBUG_SSHFILESYSTEMNODE
    qDebug() <<  "END EXPEND " + _filename;
#endif
}
