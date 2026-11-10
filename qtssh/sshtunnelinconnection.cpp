#include "sshtunnelinconnection.h"
#include "sshclient.h"
#include <QHostAddress>
#include <QTcpSocket>
#include <QEventLoop>
#include <cerrno>
#include <QTime>

Q_LOGGING_CATEGORY(logsshtunnelinconnection, "ssh.tunnelin.connection", QtWarningMsg)

#define BUFFER_LEN (16384)
#define DEBUGCH qCDebug(logsshtunnelinconnection) << m_name




SshTunnelInConnection::SshTunnelInConnection(const QString &name, SshClient *client)
    : SshChannel(name, client)
    , m_connector(client, name)
{
    QObject::connect(&m_sock, &QTcpSocket::connected, this, &SshTunnelInConnection::_socketConnected);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    QObject::connect(&m_sock, &QAbstractSocket::errorOccurred, this, &SshTunnelInConnection::_socketError);
#else
    QObject::connect(&m_sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &SshTunnelInConnection::_socketError);
#endif
    QObject::connect(this, &SshTunnelInConnection::sendEvent, this, &SshTunnelInConnection::_eventLoop, Qt::QueuedConnection);
    QObject::connect(&m_connector, &SshTunnelDataConnector::sendEvent, this, &SshTunnelInConnection::sendEvent);
}

void SshTunnelInConnection::configure(LIBSSH2_CHANNEL *channel, quint16 port, QString hostname)
{
    DEBUGCH << "configure: " << hostname << ":" << port;
    m_sshChannel = channel;
    m_port = port;
    m_hostname = hostname;
    _eventLoop();
}

SshTunnelInConnection::~SshTunnelInConnection()
{
    DEBUGCH << "SshTunnelInConnection Destroyed";
}

void SshTunnelInConnection::close()
{
    if(channelState() != ChannelState::Error && channelState() != ChannelState::Free)
    {
        setChannelState(ChannelState::Close);
    }
    emit sendEvent();
}

void SshTunnelInConnection::_socketError()
{
    if(channelState() == ChannelState::Exec)
    {
        /* Local connection refused: nothing will ever come, release the ssh channel */
        qCWarning(logsshtunnelinconnection) << m_name << "local connection failed:" << m_sock.errorString();
        m_connectFailed = true;
        setChannelState(ChannelState::Close);
        emit sendEvent();
    }
}

void SshTunnelInConnection::_eventLoop()
{
    switch(channelState())
    {
        case Openning:
        {
            DEBUGCH << "Channel session opened:" << m_hostname << ":" << m_port;
            m_sock.connectToHost(m_hostname, m_port);
            setChannelState(SshChannel::Exec);
            return;
        }

        case Exec:
        {
            // Wait connected, state change by _socketConnected
            return;
        }
        case Ready:
        {
            if(!m_connector.process())
            {
                setChannelState(ChannelState::Close);
            }
            return;
        }

        case Close:
        {
            m_connector.close();
            setChannelState(ChannelState::WaitClose);
        }

        FALLTHROUGH; case WaitClose:
        {
            DEBUGCH << "Wait close channel";
            if(!m_connectFailed && !m_connector.isClosed())
            {
                m_connector.process();
                if(!m_connector.isClosed())
                {
                    return;
                }
            }
            setChannelState(ChannelState::Freeing);
        }

        FALLTHROUGH; case Freeing:
        {
            DEBUGCH << "Freeing Channel";

            int ret = m_sshChannel ? libssh2_channel_free(m_sshChannel) : 0;
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if(ret < 0)
            {
                if(!m_error)
                {
                    m_error = true;
                    qCWarning(logsshtunnelinconnection) << "Failed to free channel: " << sshErrorToString(ret);
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
            QObject::disconnect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelInConnection::sshDataReceived);
            return;
        }

        case Free:
        {
            qCDebug(logsshtunnelinconnection) << "Channel" << m_name << "is free";
            return;
        }

        case Error:
        {
            qCDebug(logsshtunnelinconnection) << "Channel" << m_name << "is in error state";
            setChannelState(ChannelState::Free);
            sshDataReceived();
            return;
        }
    }
}

void SshTunnelInConnection::sshDataReceived()
{
    DEBUGCH << "sshDataReceived: SSH data received";
    m_connector.sshDataReceived();
    emit sendEvent();
}

void SshTunnelInConnection::flushTx()
{
    m_connector.flushTx();
}

void SshTunnelInConnection::_socketConnected()
{
    DEBUGCH << "Socket connection established";
    m_connector.setChannel(m_sshChannel);
    m_connector.setSock(&m_sock);
    setChannelState(ChannelState::Ready);
    emit sendEvent();
}


void SshTunnelInConnection::sessionClosed()
{
    m_sshChannel = nullptr;
    m_connector.setChannel(nullptr);
    SshChannel::sessionClosed();
}
