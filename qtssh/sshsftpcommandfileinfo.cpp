#include "sshsftpcommandfileinfo.h"
#include "sshclient.h"

LIBSSH2_SFTP_ATTRIBUTES SshSftpCommandFileInfo::fileinfo() const
{
    return m_fileinfo;
}

SshSftpCommandFileInfo::SshSftpCommandFileInfo(const QString &path, SshSFtp &parent)
    : SshSftpCommand(parent)
    , m_path(path)
{
    setName(QString("unlink(%1)").arg(path));
}



bool SshSftpCommandFileInfo::error() const
{
    return m_error;
}

void SshSftpCommandFileInfo::process()
{
    int res;
    switch(m_state)
    {
    case Openning:
        res = libssh2_sftp_stat_ex(
                    sftp().getSftpSession(),
                    qPrintable(m_path),
                    static_cast<unsigned int>(m_path.size()),
                    LIBSSH2_SFTP_STAT,
                    &m_fileinfo
                    );

        if(res < 0)
        {
            if(res == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            m_error = true;
            qCWarning(logsshsftp) << "SFTP unlink error " << res;
            setState(CommandState::Error);
            break;
        }
        setState(CommandState::Terminate);
        FALLTHROUGH;
    case Terminate:
        break;

    case Error:
        break;

    default:
        setState(CommandState::Terminate);
        break;
    }
}
