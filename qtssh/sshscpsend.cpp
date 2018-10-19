#include "sshscpsend.h"
#include "sshclient.h"
#include <QFileInfo>
#include <qdebug.h>

SshScpSend::SshScpSend(SshClient *client):
    SshChannel(client)
{

}

#if !defined(PAGE_SIZE)
#define PAGE_SIZE (4*1024)
#endif

QString SshScpSend::send(const QString &source, QString dest)
{
    struct stat fileinfo;
    QFileInfo src(source);
    if(dest.at(dest.length() - 1) != '/')
    {
        dest.append("/");
    }
    QString destination = dest + src.fileName();

    stat(source.toStdString().c_str(), &fileinfo);
    if(!sshChannel)
    {
        sshChannel = qssh2_scp_send64(sshClient->session(), destination.toStdString().c_str(), fileinfo.st_mode & 0777, fileinfo.st_size, 0, 0);
    }

    QFile qsource(source);
    if(qsource.open(QIODevice::ReadOnly))
    {
        QByteArray buf;
        while(qsource.atEnd())
        {
            qint64 offset = 0;
            qint64 readsize;
            char buffer[PAGE_SIZE];
            readsize = qsource.read(buffer, PAGE_SIZE);

            while(offset < readsize)
            {
                ssize_t ret = qssh2_channel_write(sshChannel, buffer + offset, static_cast<size_t>(readsize - offset));
                if(ret < 0)
                {
                    qDebug() << "ERROR : send file return " << ret;
                    return QString();
                }
                else
                {
                    offset += ret;
                }
            }
        }
    }
    qsource.close();

    int ret = qssh2_channel_send_eof(sshChannel);
    if(ret < 0)
    {
        qDebug() << "ERROR : SendFile failed: Can't send EOF";
    }

    return destination;
}

