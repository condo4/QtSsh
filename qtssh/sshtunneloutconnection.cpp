#include "sshtunneloutconnection.h"
#include "sshtunnelout.h"
#include "sshclient.h"

Q_LOGGING_CATEGORY(logsshtunneloutconnection, "ssh.tunnelout.connection")
Q_LOGGING_CATEGORY(logsshtunneloutconnectiontransfer, "ssh.tunnelout.connection.transfer")

#define DEBUGCH qCDebug(logsshtunneloutconnection) << m_name
#define DEBUGTX qCDebug(logsshtunneloutconnectiontransfer) << m_name

#define SOCKET_WRITE_ERROR (-1001)

SshTunnelOutConnection::SshTunnelOutConnection(const QString &name, SshClient *client, QTcpServer &server, quint16 remotePort, QString target)
    : SshChannel(name, client)
    , m_server(server)
    , m_port(remotePort)
    , m_target(target)
    , m_tx_stop_ptr(m_tx_buffer)
{
    QObject::connect(this, &SshTunnelOutConnection::sendEvent, this, &SshTunnelOutConnection::_eventLoop, Qt::QueuedConnection);
    DEBUGCH << "Create SshTunnelOutConnection (constructor)";
    emit sendEvent();
}

SshTunnelOutConnection::~SshTunnelOutConnection()
{
    DEBUGCH << "Free SshTunnelOutConnection (destructor)";
}

void SshTunnelOutConnection::close()
{
    DEBUGCH << "Close SshTunnelOutConnection asked";
    setChannelState(ChannelState::Close);
    emit sendEvent();
}

int SshTunnelOutConnection::_displaySshError(const QString &msg)
{
    char *emsg;
    int size;
    int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
    if(ret == LIBSSH2_ERROR_EAGAIN)
    {
        /* Process next connection */
        return LIBSSH2_ERROR_EAGAIN;
    }
    qCCritical(logsshtunneloutconnection) << m_name << "Error" << ret << msg << QString(emsg);
    return ret;
}


ssize_t SshTunnelOutConnection::_transferSockToTx()
{
    qint64 len = 0;
    if(m_tx_stop_ptr != nullptr)
    {
        qCWarning(logsshtunneloutconnection) << m_name << "Asking transfer sock to tx when buffer not empty (" << m_tx_stop_ptr - m_tx_start_ptr << " bytes)";
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
        m_tx_start_ptr = m_tx_buffer;
        if(len < BUFFER_SIZE)
        {
            m_data_to_tx = false;
        }
        DEBUGCH << "_transferSockToTx: " << len << "bytes (available:" << m_sock->bytesAvailable() << ", state:" << m_sock->state() << ")";
        if(m_sock->bytesAvailable() == 0)
        {
            if(m_sock->state() == QAbstractSocket::UnconnectedState)
            {
                DEBUGCH << "Detect Socket disconnected";
                m_tx_closed = true;
                emit sendEvent();
            }
        }
        else
        {
            m_data_to_tx = true;
            DEBUGCH << "_transferSockToTx: There is other data in socket, re-arm read";
            emit sendEvent();
        }
    }
    else
    {
        m_tx_stop_ptr = nullptr;
        m_tx_start_ptr = nullptr;
        DEBUGCH << "_transferSockToTx: error: " << len;
    }

    return len;
}

ssize_t SshTunnelOutConnection::_transferTxToSsh()
{
    ssize_t transfered = 0;

    while(m_tx_start_ptr < m_tx_stop_ptr)
    {
        ssize_t len = libssh2_channel_write(m_sshChannel, m_tx_start_ptr, m_tx_stop_ptr - m_tx_start_ptr);
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            DEBUGCH << "_transferTxToSsh: write again" ;
            return 0;
        }
        if (len < 0)
        {
            DEBUGCH << "_transferTxToSsh: ERROR" << len;
            return _displaySshError("libssh2_channel_write");
        }
        if (len == 0)
        {
            qCWarning(logsshtunneloutconnectiontransfer) << m_name << "ERROR:  libssh2_channel_write return 0";
            return 0;
        }
        /* xfer OK */
        m_tx_start_ptr += len;
        transfered += len;
        DEBUGCH << "_transferTxToSsh: write on SSH return " << len << "bytes" ;

        if(m_tx_start_ptr == m_tx_stop_ptr)
        {
            DEBUGCH << "_transferTxToSsh: All buffer sent on SSH, buffer empty" ;
            m_tx_stop_ptr = nullptr;
            m_tx_start_ptr = nullptr;
        }
    }
    return transfered;
}

ssize_t SshTunnelOutConnection::_transferSshToRx()
{
    ssize_t sshread = 0;

    if(m_rx_stop_ptr != nullptr)
    {
        qCWarning(logsshtunneloutconnection) << "Buffer not empty, need to retry later";
        emit sendEvent();
        return 0;
    }

    sshread = static_cast<ssize_t>(libssh2_channel_read(m_sshChannel, m_rx_buffer, BUFFER_SIZE));
    if (sshread < 0)
    {
        if(sshread != LIBSSH2_ERROR_EAGAIN)
        {
            DEBUGCH << "_transferSshToRx: " << sshread << " (error)";
            m_rx_stop_ptr = nullptr;
            _displaySshError(QString("libssh2_channel_read (%1 / %2)").arg(sshread).arg(BUFFER_SIZE));
        }
        else
        {
            DEBUGCH << "_transferSshToRx: LIBSSH2_ERROR_EAGAIN";
            m_data_to_rx = false;
        }
        return sshread;
    }

    if(sshread < BUFFER_SIZE)
    {
        DEBUGCH << "_transferSshToRx: Xfer " << sshread << "bytes";
        m_data_to_rx = false;
        if (libssh2_channel_eof(m_sshChannel))
        {
            m_rx_closed = true;
            DEBUGCH << "_transferSshToRx: Ssh channel closed";
        }
    }
    else
    {
        DEBUGCH << "_transferSshToRx: Xfer " << sshread << "bytes; There is probably more data to read, re-arm event";
        emit sendEvent();
    }
    m_rx_stop_ptr = m_rx_buffer + sshread;
    m_rx_start_ptr = m_rx_buffer;
    return sshread;
}

ssize_t SshTunnelOutConnection::_transferRxToSock()
{
    ssize_t total = 0;

    /* If socket not ready, wait for socket connected */
    if(!m_sock || m_sock->state() != QAbstractSocket::ConnectedState)
    {
        qCWarning(logsshtunneloutconnection) << m_name << "_transferRxToSock: Data on SSH when socket closed";
        return -1;
    }

    if(m_rx_stop_ptr == nullptr)
    {
        qCWarning(logsshtunneloutconnection) <<  m_name << "Buffer empty";
        return 0;
    }

    DEBUGCH << "_transferRxToSock: libssh2_channel_read return " << (m_rx_stop_ptr - m_rx_start_ptr) << "bytes";

    while (m_rx_start_ptr < m_rx_stop_ptr)
    {
        ssize_t slen = m_sock->write(m_rx_start_ptr, m_rx_stop_ptr - m_rx_start_ptr);
        if (slen <= 0)
        {
            qCWarning(logsshtunneloutconnectiontransfer) << "ERROR : " << m_name << " local failed to write (" << slen << ")";
            return slen;
        }
        m_rx_start_ptr += slen;
        total += slen;
        DEBUGCH << "_transferRxToSock: " << slen << "bytes written on socket";
    }

    /* Buffer is empty */
    m_rx_stop_ptr = nullptr;
    m_rx_start_ptr = nullptr;
    return total;
}


void SshTunnelOutConnection::_socketDisconnected()
{
    DEBUGCH << "_socketDisconnected: Socket disconnected";
    if(m_sock->bytesAvailable() == 0)
    {
        m_tx_closed = true;
        emit sendEvent();
    }
}

void SshTunnelOutConnection::_socketDataRecived()
{
    DEBUGCH << "_socketDataRecived: Socket data received";
    m_data_to_tx = true;
    emit sendEvent();
}

void SshTunnelOutConnection::sshDataReceived()
{
    DEBUGCH << "sshDataReceived: SSH data received";
    m_data_to_rx = true;
    emit sendEvent();
}

void SshTunnelOutConnection::_socketError()
{
    DEBUGCH << "_socketError";
    auto error = m_sock->error();
    switch(error)
    {
        case QAbstractSocket::RemoteHostClosedError:
            DEBUGCH << "socket RemoteHostClosedError, data available:" << m_sock->bytesAvailable();
            // Socket will be closed just after this, nothing to care about
            break;
        default:
            qCWarning(logsshtunneloutconnection) << m_name << "socket error=" << error << m_sock->errorString();
            // setChannelState(ChannelState::Close);
    }
}

void SshTunnelOutConnection::_eventLoop()
{
    switch(channelState())
    {
        case Openning:
        {
            if ( ! m_sshClient->takeChannelCreationMutex(this) )
            {
                return;
            }
            m_sshChannel = libssh2_channel_direct_tcpip(m_sshClient->session(), qPrintable(m_target), m_port);
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
            DEBUGCH << "Channel session opened";
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
            DEBUGCH << "createConnection: " << m_sock << m_sock->localPort();
            m_rx_start_ptr = nullptr;
            m_rx_stop_ptr = nullptr;
            m_tx_start_ptr = nullptr;
            m_tx_stop_ptr = nullptr;
            m_data_to_tx = true;
            setChannelState(ChannelState::Read);
            /* OK, next step */
        }

        FALLTHROUGH; case Read:
        {
            DEBUGCH << "_eventLoop in" << channelState() << "RX:" << m_data_to_rx << " TX:" << m_data_to_tx << " BUFTX:" << (m_tx_stop_ptr - m_tx_start_ptr)  << " BUFRX:" << (m_rx_stop_ptr - m_rx_start_ptr);
            ssize_t xfer = 0;
            if(m_rx_start_ptr) xfer += _transferRxToSock();
            if(m_data_to_rx)   _transferSshToRx();
            if(m_rx_start_ptr) xfer += _transferRxToSock();

            if(m_tx_start_ptr) xfer += _transferTxToSsh();
            if(m_data_to_tx)   _transferSockToTx();
            if(m_tx_start_ptr) xfer += _transferTxToSsh();

            if(m_tx_closed && (m_tx_start_ptr == nullptr))
            {
                DEBUGCH << "Send EOF to SSH";
                libssh2_channel_send_eof(m_sshChannel);
                setChannelState(ChannelState::Close);
            }
            if(m_rx_closed && (m_rx_start_ptr == nullptr))
            {
                DEBUGCH << "Send EOF to Socket";
                m_sock->disconnectFromHost();
                setChannelState(ChannelState::Close);
            }

            DEBUGCH << "_eventLoop out:" << channelState() << "RX:" << m_data_to_rx << " TX:" << m_data_to_tx << " BUFTX:" << (m_tx_stop_ptr - m_tx_start_ptr)  << " BUFRX:" << (m_rx_stop_ptr - m_rx_start_ptr);
            return;
        }

        case Close:
        {
            if(!m_disconnectedFromSock && m_sock && m_sock->state() == QAbstractSocket::ConnectedState)
            {
                m_sock->disconnectFromHost();
            }
            DEBUGCH << "closeChannel";
            setChannelState(ChannelState::WaitClose);
        }

        FALLTHROUGH; case WaitClose:
        {
            DEBUGCH << "Wait close channel";
            if(m_sock->state() == QAbstractSocket::UnconnectedState)
            {
                setChannelState(ChannelState::Freeing);
            }
        }

        FALLTHROUGH; case Freeing:
        {
            DEBUGCH << "free Channel";

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

