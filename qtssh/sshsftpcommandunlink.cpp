#include "sshsftpcommandunlink.h"
#include "sshclient.h"

SshSftpCommandUnlink::SshSftpCommandUnlink(const QString &path, SshSFtp &parent)
    : SshSftpCommand(parent)
    , m_path(path)
{
    setName(QString("unlink(%1)").arg(path));
}

bool SshSftpCommandUnlink::error() const
{
    return m_error;
}

void SshSftpCommandUnlink::process()
{
    int res;
    switch(m_state)
    {
    case Openning:
        res = libssh2_sftp_unlink_ex(
                    sftp().getSftpSession(),
                    qPrintable(m_path),
                    static_cast<unsigned int>(m_path.size())
                    );

        if(res < 0)
        {
            if(res == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            m_error = true;
            qCWarning(logsshsftp) << "SFTP unlink error " << res;
        }
        else
        {
            setState(CommandState::Terminate);
            FALLTHROUGH;
        }
    case Terminate:
        break;

    case Error:
        break;

    default:
        setState(CommandState::Terminate);
        break;
    }
}
