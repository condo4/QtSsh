#include "sshscpget.h"
#include "sshclient.h"
#include <QFileInfo>
#include <qdebug.h>

SshScpGet::SshScpGet(const QString &name, SshClient *client):
    SshChannel(name, client)
{

}

SshScpGet::~SshScpGet()
{
    free();
}

void SshScpGet::free()
{
    qCDebug(sshchannel) << "free Channel:" << m_name;
    if (m_sshChannel == nullptr)
        return;

    close();

    qCDebug(sshchannel) << "freeChannel:" << m_name;

    int ret = qssh2_channel_free(m_sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_free";
        return;
    }
    m_sshChannel = nullptr;
}

void SshScpGet::close()
{
    if (m_sshChannel == nullptr || !m_sshClient->getSshConnected())
        return;

    qCDebug(sshchannel) << "closeChannel:" << m_name;
    int ret = qssh2_channel_close(m_sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
        return;
    }

    ret = qssh2_channel_wait_closed(m_sshChannel);
    if(ret)
    {
        qCDebug(sshchannel) << "Failed to channel_wait_closed";
        return;
    }

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
        m_sshChannel = qssh2_scp_recv2(m_sshClient->session(), qPrintable(source), &fileinfo);
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
