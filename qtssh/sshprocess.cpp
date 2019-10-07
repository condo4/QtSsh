#include "sshprocess.h"
#include "sshclient.h"
#include <QTimer>
#include <QEventLoop>

Q_LOGGING_CATEGORY(logsshprocess, "ssh.process", QtWarningMsg)

SshProcess::SshProcess(const QString &name, SshClient *client)
    : SshChannel(name, client)
{
    QObject::connect(m_sshClient, &SshClient::sshDataReceived, this, &SshProcess::sshDataReceived);
}

SshProcess::~SshProcess()
{
    qCDebug(sshchannel) << "free Channel:" << m_name;
}

void SshProcess::close()
{
    setChannelState(ChannelState::Close);
    sshDataReceived();
}

QByteArray SshProcess::result()
{
    return m_result;
}

bool SshProcess::isError()
{
    return m_error;
}

void SshProcess::runCommand(const QString &cmd)
{
    m_cmd = cmd;
    sshDataReceived();
}

void SshProcess::sshDataReceived()
{
    qCDebug(logsshprocess) << "Channel "<< m_name << "State:" << channelState();
    switch(channelState())
    {
        case Openning:
        {
            m_sshChannel = libssh2_channel_open_ex(m_sshClient->session(), "session", sizeof("session") - 1, LIBSSH2_CHANNEL_WINDOW_DEFAULT, LIBSSH2_CHANNEL_PACKET_DEFAULT, nullptr, 0);
            if (m_sshChannel == nullptr)
            {
                int ret = libssh2_session_last_error(m_sshClient->session(), nullptr, nullptr, 0);
                if(ret == LIBSSH2_ERROR_EAGAIN)
                {
                    return;
                }
                if(!m_error)
                {
                    m_error = true;
                    emit failed();
                }
                setChannelState(ChannelState::Error);
                qCWarning(logsshprocess) << "Channel session open failed";
                return;
            }
            m_sshClient->registerChannel(this);
            qCDebug(logsshprocess) << "Channel session opened";
            setChannelState(ChannelState::Exec);
        }

        FALLTHROUGH; case Exec:
        {
            if(m_cmd.size() == 0)
            {
                /* Nothing to process */
                return;
            }
            qCDebug(logsshprocess) << "runCommand(" << m_cmd << ")";
            int ret = libssh2_channel_process_startup(m_sshChannel, "exec", sizeof("exec") - 1, m_cmd.toStdString().c_str(), static_cast<unsigned int>(m_cmd.size()));
            if (ret == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if(ret != 0)
            {
                if(!m_error)
                {
                    m_error = true;
                    emit failed();
                    qCWarning(logsshprocess) << "Failed to run command" << ret;
                }
                setChannelState(ChannelState::Close);
                sshDataReceived();
                return;
            }
            setChannelState(ChannelState::Read);
            /* OK, next step */
        }

        FALLTHROUGH; case Read:
        {
            ssize_t retsz;
            char buffer[16*1024];

            retsz = libssh2_channel_read_ex(m_sshChannel, 0, buffer, 16 * 1024);
            if(retsz == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }

            if(retsz < 0)
            {
                if(!m_error)
                {
                    m_error = true;
                    emit failed();
                    qCWarning(logsshprocess) << "Can't read result (" << sshErrorToString(static_cast<int>(retsz)) << ")";
                }
                setChannelState(ChannelState::Close);
                sshDataReceived();
                return;
            }

            m_result.append(buffer, static_cast<int>(retsz));

            if (libssh2_channel_eof(m_sshChannel) == 1)
            {
                qCDebug(logsshprocess) << "runCommand(" << m_cmd << ") RESULT: " << m_result;
                setChannelState(Close);
                emit finished();
            }
        }

        FALLTHROUGH; case Close:
        {
            qCDebug(logsshprocess) << "closeChannel:" << m_name;
            int ret = libssh2_channel_close(m_sshChannel);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if(ret < 0)
            {
                if(!m_error)
                {
                    m_error = true;
                    emit failed();
                    qCWarning(logsshprocess) << "Failed to channel_close: " << sshErrorToString(ret);
                }
            }
            setChannelState(ChannelState::WaitClose);
        }

        FALLTHROUGH; case WaitClose:
        {
            qCDebug(logsshprocess) << "Wait close channel:" << m_name;
            int ret = libssh2_channel_wait_closed(m_sshChannel);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if(ret < 0)
            {
                if(!m_error)
                {
                    m_error = true;
                    emit failed();
                    qCWarning(logsshprocess) << "Failed to channel_wait_close: " << sshErrorToString(ret);
                }
            }
            setChannelState(ChannelState::Freeing);
        }

        FALLTHROUGH; case Freeing:
        {
            qCDebug(logsshprocess) << "free Channel:" << m_name;

            int ret = libssh2_channel_free(m_sshChannel);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if(ret < 0)
            {
                if(!m_error)
                {
                    m_error = true;
                    emit failed();
                    qCWarning(logsshprocess) << "Failed to free channel: " << sshErrorToString(ret);
                }
            }
            if(m_error)
            {
                setChannelState(ChannelState::Error);
            }
            else
            {
                setChannelState(ChannelState::Free);
            }
            m_sshChannel = nullptr;
            QObject::disconnect(m_sshClient, &SshClient::sshDataReceived, this, &SshProcess::sshDataReceived);
            emit canBeDestroy(this);
            return;
        }

        case Free:
        {
            qCDebug(logsshprocess) << "Channel" << m_name << "is free";
            return;
        }

        case Error:
        {
            emit failed();
            qCDebug(logsshprocess) << "Channel" << m_name << "is in error state";
            return;
        }
    }
}
