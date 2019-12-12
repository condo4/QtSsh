#include "sshtunneloutconnection.h"
#include "sshtunnelout.h"
#include "sshclient.h"

Q_LOGGING_CATEGORY(logsshtunneloutconnection, "ssh.tunnelout.connection")
Q_LOGGING_CATEGORY(logsshtunneloutconnectiontransfer, "ssh.tunnelout.connection.transfer")

#define SOCKET_WRITE_ERROR (-1001)

SshTunnelOutConnection::SshTunnelOutConnection(const QString &name, SshClient *client, QTcpServer &server, quint16 remotePort)
    : SshChannel(name, client)
    , m_server(server)
    , m_port(remotePort)
    , m_tx_stop_ptr(m_tx_buffer)
{
    _eventLoop(__FUNCTION__);
}

SshTunnelOutConnection::~SshTunnelOutConnection()
{
    qCDebug(logsshtunneloutconnection) << "free Channel:" << m_name;
}

void SshTunnelOutConnection::close()
{
    qCDebug(logsshtunneloutconnection) << m_name << "close Channel asked";
    setChannelState(ChannelState::Close);
    _eventLoop(__FUNCTION__);
}


int SshTunnelOutConnection::_displaySshError(const QString &msg)
{
    char *emsg;
    int size;
    int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
    if(ret == LIBSSH2_ERROR_EAGAIN)
    {
        /* Process next connection */
        m_sshWaiting = true;
        return LIBSSH2_ERROR_EAGAIN;
    }
    qCCritical(logsshtunneloutconnection) << m_name << "Error" << ret << msg << QString(emsg);
    return ret;
}


ssize_t SshTunnelOutConnection::_transferSockToTx()
{
    qint64 len = 0;
    if(m_tx_buffer != m_tx_stop_ptr)
    {
        qCWarning(logsshtunneloutconnection) << m_name << "Asking transfer sock to tx when buffer not ready";
        return -1;
    }

    if(m_sock == nullptr && m_sock->state() != QAbstractSocket::ConnectedState)
    {
        qCCritical(logsshtunneloutconnectiontransfer) << m_name << "_transferSockToTx on invalid socket";
        return -1;
    }

    len = m_sock->read(m_tx_buffer, BUFFER_SIZE);
    if(len > 0)
    {
        m_tx_stop_ptr = m_tx_buffer + len;
        qCDebug(logsshtunneloutconnectiontransfer) << m_name << " read on socket return " << len << "bytes (total:" << m_readen << ", available:" << m_sock->bytesAvailable() << ")";
        m_dataWaitingOnSock = true;
        _transferTxToSsh();
    }
    else if(len < 0)
    {
        m_dataWaitingOnSock = true;
        qCWarning(logsshtunneloutconnectiontransfer) << m_name << " read on socket return error:" << len << m_sock->errorString();
    }
    else if(len == 0)
    {
        m_dataWaitingOnSock = false;
        qCWarning(logsshtunneloutconnectiontransfer) << m_name << "No more data available";
        if(m_disconnectedFromSock)
        {
            setChannelState(ChannelState::Close);
        }
    }

    return len;
}

ssize_t SshTunnelOutConnection::_transferTxToSsh()
{
    char *start = m_tx_buffer;
    char *end = m_tx_stop_ptr;
    ssize_t transfered = 0;

    while(start < end)
    {
        ssize_t len = libssh2_channel_write(m_sshChannel, start, static_cast<size_t>(end - start));
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            qCDebug(logsshtunneloutconnectiontransfer) << m_name << " write again" ;
            return 0;
        }
        if (len < 0)
        {
            return _displaySshError("libssh2_channel_write");
        }
        if (len == 0)
        {
            qCWarning(logsshtunneloutconnectiontransfer) << "ERROR : " << m_name << " libssh2_channel_write return 0";
            return 0;
        }
        /* xfer OK */
        start += len;
        transfered += len;
        qCDebug(logsshtunneloutconnectiontransfer) << m_name << " write on SSH return " << len << "bytes" ;

        if(start == end)
        {
            qCDebug(logsshtunneloutconnectiontransfer) << m_name << " All buffer sent on SSH, buffer empty" ;
            m_tx_stop_ptr = m_tx_buffer;
            if(m_dataWaitingOnSock)
                _transferSockToTx();
        }
    }
    return transfered;
}


void SshTunnelOutConnection::_socketDisconnected()
{
    qCDebug(logsshtunneloutconnection) << "Set Disconnected from socket";
    m_disconnectedFromSock = true;
    setChannelState(ChannelState::Close);
    _eventLoop(__FUNCTION__);
}

void SshTunnelOutConnection::_socketDataRecived()
{
    if(m_tx_stop_ptr != m_tx_buffer)
    {
        qCDebug(logsshtunneloutconnection) << "SOCKET DATA RECIVED but buffer not empty";
        m_dataWaitingOnSock = true;
        return;
    }
    qCDebug(logsshtunneloutconnection) << "SOCKET DATA RECIVED and buffer empty, start transfer";
    _transferSockToTx();
}


void SshTunnelOutConnection::sshDataReceived()
{
    static char rxBuffer[BUFFER_SIZE];

    if(channelState() == ChannelState::Read)
    {
        if(m_tx_buffer != m_tx_stop_ptr)
        {
            _transferTxToSsh();
        }

        ssize_t sshread = 0;
        ssize_t total = 0;

        /* If socket not ready, wait for socket connected */
        if(!m_sock || m_sock->state() != QAbstractSocket::ConnectedState)
        {
            qCWarning(logsshtunneloutconnection) << "Data on SSH when socket closed";
            m_dataWaitingOnSsh = true;
            return;
        }

        do
        {
            ssize_t sshread = static_cast<ssize_t>(libssh2_channel_read(m_sshChannel, rxBuffer, BUFFER_SIZE));
            if(sshread == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if (sshread < 0)
            {
                _displaySshError(QString("libssh2_channel_read (%1 / %2)").arg(sshread).arg(BUFFER_SIZE));
                return;
            }
            if(sshread > 0)
            {
                qCDebug(logsshtunneloutconnectiontransfer) << m_name << " libssh2_channel_read return " << sshread << "bytes";

                char *rx = rxBuffer;
                char *rxend = rxBuffer + sshread;

                while (rx < rxend)
                {
                    ssize_t slen = m_sock->write(rx, rxend - rx);
                    if (slen <= 0)
                    {
                        qCWarning(logsshtunneloutconnectiontransfer) << "ERROR : " << m_name << " local failed to write (" << slen << ")";
                        return;
                    }
                    rx += slen;
                    qCDebug(logsshtunneloutconnectiontransfer) << m_name << slen << "bytes written on socket";
                }
                total += sshread;
            }
        }
        while(sshread > 0);

        if (libssh2_channel_eof(m_sshChannel))
        {
            qCDebug(logsshtunneloutconnection) << "Set Disconnected from ssh";
            m_disconnectedFromSsh = true;
            m_sock->disconnectFromHost();
            setChannelState(ChannelState::Close);
            return;
        }
    }

    _eventLoop(__FUNCTION__);
}

void SshTunnelOutConnection::_socketError()
{
    auto error = m_sock->error();
    switch(error)
    {
        case QAbstractSocket::RemoteHostClosedError:
            qCDebug(logsshtunneloutconnection) << m_name << "socket RemoteHostClosedError";
            // Socket will be closed just after this, nothing to care about
            break;
        default:
            qCWarning(logsshtunneloutconnection) << m_name << "socket error=" << error << m_sock->errorString();
            setChannelState(ChannelState::Close);
    }
}

void SshTunnelOutConnection::_eventLoop(QString why)
{
    qCDebug(logsshtunneloutconnection) << "_eventLoop for " << why << "in" << channelState();
    switch(channelState())
    {
        case Openning:
        {
            if ( ! m_sshClient->takeChannelCreationMutex(this) )
            {
                return;
            }
            m_sshChannel = libssh2_channel_direct_tcpip(m_sshClient->session(),  "127.0.0.1", m_port);
            m_sshClient->releaseChannelCreationMutex(this);
            if (m_sshChannel == nullptr)
            {
                char *emsg;
                int size;
                int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
                if(ret == LIBSSH2_ERROR_EAGAIN)
                {
                    return;
                }
                if(!m_error)
                {
                    qCDebug(logsshtunneloutconnection) << "Refuse client socket connection on " << m_server.serverPort() << QString(emsg);
                    m_error = true;
                    m_sock = m_server.nextPendingConnection();
                    if(m_sock)
                    {
                        m_sock->close();
                        m_sock->deleteLater();
                    }
                    m_server.close();
                }
                setChannelState(ChannelState::Error);
                qCWarning(logsshtunneloutconnection) << "Channel session open failed";
                return;
            }
            qCDebug(logsshtunneloutconnection) << "Channel session opened";
            setChannelState(ChannelState::Exec);
        }

        FALLTHROUGH; case Exec:
        {
            m_sock = m_server.nextPendingConnection();
            if(!m_sock)
            {
                m_server.close();
                setChannelState(ChannelState::Error);
                qCWarning(logsshtunneloutconnection) << "Fail to get client socket";
                setChannelState(ChannelState::Close);
                return;
            }

            QObject::connect(m_sock, &QTcpSocket::readyRead,
                             this,   &SshTunnelOutConnection::_socketDataRecived);

            QObject::connect(m_sock, &QTcpSocket::disconnected,
                             this,   &SshTunnelOutConnection::_socketDisconnected);

            QObject::connect(m_sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                             this,   &SshTunnelOutConnection::_socketError);

            m_name = QString(m_name + ":%1").arg(m_sock->localPort());
            qCDebug(logsshtunneloutconnection) << m_name << "createConnection: " << m_sock << m_sock->localPort();
            setChannelState(ChannelState::Read);
            /* OK, next step */
        }

        FALLTHROUGH; case Read:
        {
            if(m_dataWaitingOnSock && (m_tx_buffer == m_tx_stop_ptr))
            {
                _transferSockToTx();
            }
            _transferTxToSsh();
            return;
        }

        case Close:
        {
            if(!m_disconnectedFromSock && m_sock && m_sock->state() == QAbstractSocket::ConnectedState)
            {
                m_sock->disconnectFromHost();
            }
            qCDebug(logsshtunneloutconnection) << m_name << "closeChannel";
            setChannelState(ChannelState::WaitClose);
        }

        FALLTHROUGH; case WaitClose:
        {
            qCDebug(logsshtunneloutconnection) << "Wait close channel:" << m_name;
            if(m_sock->state() == QAbstractSocket::UnconnectedState)
            {
                setChannelState(ChannelState::Freeing);
            }
        }

        FALLTHROUGH; case Freeing:
        {
            qCDebug(logsshtunneloutconnection) << "free Channel:" << m_name;

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
                    qCWarning(logsshtunneloutconnection) << "Failed to free channel: " << sshErrorToString(ret);
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
            QObject::disconnect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelOutConnection::sshDataReceived);
            emit canBeDestroy(this);
            return;
        }

        case Free:
        {
            qCDebug(logsshtunneloutconnection) << "Channel" << m_name << "is free";
            return;
        }

        case Error:
        {
            qCDebug(logsshtunneloutconnection) << "Channel" << m_name << "is in error state";
            return;
        }
    }
}

