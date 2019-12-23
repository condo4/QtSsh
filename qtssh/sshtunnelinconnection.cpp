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




SshTunnelInConnection::SshTunnelInConnection(const QString &name, SshClient *client, LIBSSH2_CHANNEL* channel, quint16 port, QString hostname)
    : SshChannel(name, client)
    , m_sshChannel(channel)
    , m_port(port)
    , m_hostname(hostname)
{
    QObject::connect(&m_sock, &QTcpSocket::connected, this, &SshTunnelInConnection::_socketConnected);
    QObject::connect(&m_sock, &QTcpSocket::disconnected, this, &SshTunnelInConnection::_socketDisconnected);
    QObject::connect(&m_sock, &QTcpSocket::readyRead, this, &SshTunnelInConnection::_socketDataRecived);
    QObject::connect(&m_sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &SshTunnelInConnection::_socketError);
    QObject::connect(this, &SshTunnelInConnection::sendEvent, this, &SshTunnelInConnection::_eventLoop, Qt::QueuedConnection);
    _eventLoop();
}

SshTunnelInConnection::~SshTunnelInConnection()
{
}

void SshTunnelInConnection::close()
{

}

void SshTunnelInConnection::_eventLoop()
{
    switch(channelState())
    {
        case Openning:
        {
            DEBUGCH << "Channel session opened";
            m_sock.connectToHost(m_hostname, m_port);
            setChannelState(SshChannel::Exec);
            break;
        }

        case Exec:
            // Wait connected, state change by _onSockConnected
            break;

        case Read:
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
                m_sock.disconnectFromHost();
                setChannelState(ChannelState::Close);
            }

            DEBUGCH << "_eventLoop out:" << channelState() << "RX:" << m_data_to_rx << " TX:" << m_data_to_tx << " BUFTX:" << (m_tx_stop_ptr - m_tx_start_ptr)  << " BUFRX:" << (m_rx_stop_ptr - m_rx_start_ptr);
            return;
        }

    case Close:
    {
        if(!m_disconnectedFromSock && m_sock.state() == QAbstractSocket::ConnectedState)
        {
            m_sock.disconnectFromHost();
        }
        DEBUGCH << "closeChannel";
        setChannelState(ChannelState::WaitClose);
    }

    FALLTHROUGH; case WaitClose:
    {
        DEBUGCH << "Wait close channel";
        if(m_sock.state() == QAbstractSocket::UnconnectedState)
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
        emit canBeDestroy(this);
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
        return;
    }
    }
}


ssize_t SshTunnelInConnection::_transferSockToTx()
{
    qint64 len = 0;
    if(m_tx_stop_ptr != nullptr)
    {
        qCWarning(logsshtunnelinconnection) << m_name << "Asking transfer sock to tx when buffer not empty (" << m_tx_stop_ptr - m_tx_start_ptr << " bytes)";
        return -1;
    }

    if(m_sock.state() != QAbstractSocket::ConnectedState)
    {
        qCCritical(logsshtunnelinconnection) << m_name << "_transferSockToTx on invalid socket";
        return -1;
    }

    len = m_sock.read(m_tx_buffer, BUFFER_SIZE);
    if(len > 0)
    {
        m_tx_stop_ptr = m_tx_buffer + len;
        m_tx_start_ptr = m_tx_buffer;
        if(len < BUFFER_SIZE)
        {
            m_data_to_tx = false;
        }
        DEBUGCH << "_transferSockToTx: " << len << "bytes (available:" << m_sock.bytesAvailable() << ", state:" << m_sock.state() << ")";
        if(m_sock.bytesAvailable() == 0)
        {
            if(m_sock.state() == QAbstractSocket::UnconnectedState)
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

ssize_t SshTunnelInConnection::_transferTxToSsh()
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
            qCWarning(logsshtunnelinconnection) << m_name << "ERROR:  libssh2_channel_write return 0";
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

ssize_t SshTunnelInConnection::_transferSshToRx()
{
    ssize_t sshread = 0;

    if(m_rx_stop_ptr != nullptr)
    {
        qCWarning(logsshtunnelinconnection) << "Buffer not empty, need to retry later";
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

ssize_t SshTunnelInConnection::_transferRxToSock()
{
    ssize_t total = 0;

    /* If socket not ready, wait for socket connected */
    if(m_sock.state() != QAbstractSocket::ConnectedState)
    {
        qCWarning(logsshtunnelinconnection) << m_name << "_transferRxToSock: Data on SSH when socket closed";
        return -1;
    }

    if(m_rx_stop_ptr == nullptr)
    {
        qCWarning(logsshtunnelinconnection) <<  m_name << "Buffer empty";
        return 0;
    }

    DEBUGCH << "_transferRxToSock: libssh2_channel_read return " << (m_rx_stop_ptr - m_rx_start_ptr) << "bytes";

    while (m_rx_start_ptr < m_rx_stop_ptr)
    {
        ssize_t slen = m_sock.write(m_rx_start_ptr, m_rx_stop_ptr - m_rx_start_ptr);
        if (slen <= 0)
        {
            qCWarning(logsshtunnelinconnection) << "ERROR : " << m_name << " local failed to write (" << slen << ")";
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

int SshTunnelInConnection::_displaySshError(const QString &msg)
{
    char *emsg;
    int size;
    int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
    if(ret == LIBSSH2_ERROR_EAGAIN)
    {
        /* Process next connection */
        return LIBSSH2_ERROR_EAGAIN;
    }
    qCCritical(logsshtunnelinconnection) << m_name << "Error" << ret << msg << QString(emsg);
    return ret;
}

void SshTunnelInConnection::sshDataReceived()
{
    DEBUGCH << "sshDataReceived: SSH data received";
    m_data_to_rx = true;
    emit sendEvent();
}

void SshTunnelInConnection::_socketConnected()
{
    DEBUGCH << "Socket connection established";
    setChannelState(ChannelState::Read);
    emit sendEvent();
}

void SshTunnelInConnection::_socketDisconnected()
{
    DEBUGCH << "_socketDisconnected: Socket disconnected";
    if(m_sock.bytesAvailable() == 0)
    {
        m_tx_closed = true;
        emit sendEvent();
    }
}

void SshTunnelInConnection::_socketDataRecived()
{
    DEBUGCH << "_socketDataRecived: Socket data received";
    m_data_to_tx = true;
    emit sendEvent();
}

void SshTunnelInConnection::_socketError()
{
    DEBUGCH << "_socketError";
    auto error = m_sock.error();
    switch(error)
    {
        case QAbstractSocket::RemoteHostClosedError:
            DEBUGCH << "socket RemoteHostClosedError, data available:" << m_sock.bytesAvailable();
            // Socket will be closed just after this, nothing to care about
            break;
        default:
            qCWarning(logsshtunnelinconnection) << m_name << "socket error=" << error << m_sock.errorString();
            // setChannelState(ChannelState::Close);
    }
}

