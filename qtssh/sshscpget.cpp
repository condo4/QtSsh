#include "sshscpget.h"
#include "sshclient.h"
#include <QFileInfo>
#include <qdebug.h>

SshScpGet::SshScpGet(const QString &name, SshClient *client):
    SshChannel(name, client)
{

}

#ifndef PAGE_SIZE
#define PAGE_SIZE (4*1024)
#endif
QString SshScpGet::get(const QString &source, const QString &dest)
{
    libssh2_struct_stat_size got = 0;
    ssize_t rc;
    libssh2_struct_stat fileinfo = {};

    if(!m_sshChannel)
    {
        connectChannel(qssh2_scp_recv2(m_sshClient->session(), qPrintable(source), &fileinfo));
    }

    QFile qsource(dest);
    if(qsource.open(QIODevice::WriteOnly))
    {
        while(got < fileinfo.st_size) {
            char mem[PAGE_SIZE];
            int amount=sizeof(mem);

            if((fileinfo.st_size - got) < amount) {
                amount = static_cast<int>(fileinfo.st_size - got);
            }

            rc = qssh2_channel_read(m_sshChannel, mem, static_cast<size_t>(amount));

            if(rc > 0) {
                qsource.write(mem, rc);
            }
            else if(rc < 0) {
                qWarning() << "libssh2_channel_read() failed: " << rc;
                break;
            }
            got += rc;
        }
    }
    qsource.close();

    return dest;
}
