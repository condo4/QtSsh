#include "sshtunnelin.h"
#include "sshclient.h"
#include <QHostAddress>
#include <QTcpSocket>
#include <QEventLoop>
#include <cerrno>
#include <QTime>
#include <sshtunnelinconnection.h>

Q_LOGGING_CATEGORY(logsshtunnelin, "ssh.tunnelin", QtWarningMsg)

#define BUFFER_LEN (16384)


SshTunnelIn::SshTunnelIn(const QString &name, SshClient *client)
    : SshChannel(name, client)
{

}

SshTunnelIn::~SshTunnelIn()
{
}

void SshTunnelIn::listen(QString host, quint16 localPort, quint16 remotePort, QString listenHost, int queueSize)
{
    m_localTcpPort = localPort;
    m_remoteTcpPort = remotePort;
    m_queueSize = queueSize;
    m_targethost = host;
    m_listenhost = listenHost;
    m_retryListen = 10;
    setChannelState(ChannelState::Exec);
    sshDataReceived();
}

quint16 SshTunnelIn::localPort()
{
    return static_cast<unsigned short>(m_localTcpPort);
}

quint16 SshTunnelIn::remotePort()
{
    if(m_remoteTcpPort == 0) return static_cast<unsigned short>(m_boundPort);
    return static_cast<unsigned short>(m_remoteTcpPort);
}

void SshTunnelIn::sshDataReceived()
{
    switch(channelState())
    {
        case Openning:
        {
            qCDebug(logsshtunnelin) << "Channel session opened";
            break;
        }

        case Exec:
        {
            do
            {
                if ( ! m_sshClient->takeChannelCreationMutex(this) )
                {
                    return;
                }
                qCDebug(logsshtunnelin) << "Create Reverse tunnel for " << m_listenhost << m_remoteTcpPort << m_boundPort << m_queueSize;

                m_sshListener = libssh2_channel_forward_listen_ex(m_sshClient->session(), qPrintable(m_listenhost), m_remoteTcpPort, &m_boundPort, m_queueSize);
                m_sshClient->releaseChannelCreationMutex(this);

                if(m_sshListener == nullptr)
                {
                    char *emsg;
                    int size;
                    int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
                    if(ret == LIBSSH2_ERROR_EAGAIN)
                    {
                        return;
                    }
                    if ( ret==LIBSSH2_ERROR_REQUEST_DENIED && m_retryListen > 0 )
                    {
                        m_retryListen--;
                        return;
                    }

                    setChannelState(ChannelState::Error);
                    qCWarning(logsshtunnelin) << "Channel session open failed: " << emsg;
                    return;
                }
            } while (m_sshListener == nullptr);
            /* OK, next step */
            setChannelState(ChannelState::Ready);
        }

        FALLTHROUGH; case Ready:
        {
            LIBSSH2_CHANNEL *newChannel;
            if ( ! m_sshClient->takeChannelCreationMutex(this) )
            {
                return;
            }
             qCDebug(logsshtunnelin) << "SshTunnelIn Listen on " << m_boundPort;
            newChannel = libssh2_channel_forward_accept(m_sshListener);
            m_sshClient->releaseChannelCreationMutex(this);

            if(newChannel == nullptr)
            {
                char *emsg;
                int size;
                int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
                if(ret == LIBSSH2_ERROR_EAGAIN)
                {
                    return;
                }

                qCWarning(logsshtunnelin) << "Channel session open failed: " << emsg;
                return;
            }

            /* We have a new connection on the remote port, need to create a connection tunnel */
            qCDebug(logsshtunnelin) << "SshTunnelIn new connection";
            SshTunnelInConnection *connection = m_sshClient->getChannel<SshTunnelInConnection>(m_name + QString("_%1").arg(m_connectionCounter++));
            connection->configure(newChannel, m_localTcpPort, m_targethost);
            m_connection.append(connection);
            QObject::connect(connection, &SshTunnelInConnection::stateChanged, this, &SshTunnelIn::connectionStateChanged);
            emit connectionChanged(m_connection.size());
            break;
        }

        case Close:
        {

            if(m_sshListener)
            {
                libssh2_channel_forward_cancel(m_sshListener);
                m_sshListener = nullptr;
            }
        }

        FALLTHROUGH; case WaitClose:
        {
            qCDebug(logsshtunnelin) << "Wait close channel";
            setChannelState(ChannelState::Freeing);
        }
        FALLTHROUGH; case Freeing:
        {
            qCDebug(logsshtunnelin) << "free Channel";
            setChannelState(ChannelState::Freeing);
        }

        FALLTHROUGH; case Free:
        {
            qCDebug(logsshtunnelin) << "Channel" << m_name << "is free";
            return;
        }

        case Error:
        {
            qCDebug(logsshtunnelin) << "Channel" << m_name << "is in error state";
            return;
        }
    }
}

void SshTunnelIn::connectionStateChanged()
{
    QObject *obj = QObject::sender();
    SshTunnelInConnection *connection = qobject_cast<SshTunnelInConnection*>(obj);
    if(connection)
    {
        if(connection->channelState() == SshChannel::ChannelState::Free)
        {
            m_connection.removeAll(connection);
            emit connectionChanged(m_connection.count());
        }
    }
}

void SshTunnelIn::close()
{
    setChannelState(ChannelState::Close);
    sshDataReceived();
}
