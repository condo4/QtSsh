#include "sshscpsend.h"
#include "sshclient.h"
#include <QFileInfo>
#include <qdebug.h>

SshScpSend::SshScpSend(const QString &name, SshClient *client):
    SshChannel(name, client)
{

}

#if !defined(PAGE_SIZE)
#define PAGE_SIZE (4*1024)
#endif

QString SshScpSend::send(const QString &source, QString dest)
{
    struct stat fileinfo = {};
    QFileInfo src(source);
    if(dest.at(dest.length() - 1) != '/')
    {
        dest.append("/");
    }
    QString destination = dest + src.fileName();

    stat(source.toStdString().c_str(), &fileinfo);
    if(!m_sshChannel)
    {
        connectChannel(qssh2_scp_send64(m_sshClient->session(), destination.toStdString().c_str(), fileinfo.st_mode & 0777, fileinfo.st_size, 0, 0));
    }

    QFile qsource(source);
    if(qsource.open(QIODevice::ReadOnly))
    {
        while(qsource.atEnd())
        {
            qint64 offset = 0;
            qint64 readsize;
            char buffer[PAGE_SIZE];
            readsize = qsource.read(buffer, PAGE_SIZE);

            while(offset < readsize)
            {
                ssize_t ret = qssh2_channel_write(m_sshChannel, buffer + offset, static_cast<size_t>(readsize - offset));
                if(ret < 0)
                {
                    qDebug() << "ERROR : send file return " << ret;
                    return QString();
                }
                offset += ret;
            }
        }
    }
    qsource.close();

    int ret = qssh2_channel_send_eof(m_sshChannel);
    if(ret < 0)
    {
        qDebug() << "ERROR : SendFile failed: Can't send EOF";
    }

    return destination;
}

