#include "sshtunneldataconnector.h"
#include <QTcpSocket>
#include "sshclient.h"

Q_LOGGING_CATEGORY(logxfer, "ssh.tunnel.transfer")
#define DEBUGCH qCDebug(logxfer) << m_name

SshTunnelDataConnector::SshTunnelDataConnector(SshClient *client, QString name, LIBSSH2_CHANNEL *channel, QTcpSocket *sock, QObject *parent)
    : QObject(parent)
    , m_sshClient(client)
    , m_sshChannel(channel)
    , m_sock(sock)
    , m_name(name)
{
    QObject::connect(m_sock, &QTcpSocket::disconnected,
                     this,   &SshTunnelDataConnector::_socketDisconnected);


    QObject::connect(m_sock, &QTcpSocket::readyRead,
                     this,   &SshTunnelDataConnector::_socketDataRecived);



    QObject::connect(m_sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                     this,   &SshTunnelDataConnector::_socketError);

    QObject::connect(m_sock, &QTcpSocket::bytesWritten, this, [this](qint64 len){ m_input_real += len; });

}

SshTunnelDataConnector::~SshTunnelDataConnector()
{
    QObject::disconnect(m_sock);
    m_sock->deleteLater();
    DEBUGCH << "TOTAL TRANSFERED: " << m_output << " " << m_input_real << " / " << m_input;
}

void SshTunnelDataConnector::_socketDisconnected()
{
    DEBUGCH << "_socketDisconnected: Socket disconnected";
    if(m_sock->bytesAvailable() == 0)
    {
        m_tx_closed = true;
        emit sendEvent();
    }
}

void SshTunnelDataConnector::_socketDataRecived()
{
    DEBUGCH << "_socketDataRecived: Socket data received";
    m_data_to_tx = true;
    emit sendEvent();
}

void SshTunnelDataConnector::_socketError()
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
            qCWarning(logxfer) << m_name << "socket error=" << error << m_sock->errorString();
            // setChannelState(ChannelState::Close);
    }
}

ssize_t SshTunnelDataConnector::_transferSockToTx()
{
    qint64 len = 0;
    if(m_tx_stop_ptr != nullptr)
    {
        qCDebug(logxfer) << m_name << "Asking transfer sock to tx when buffer not empty (" << m_tx_stop_ptr - m_tx_start_ptr << " bytes)";
        return -1;
    }

    if(m_sock == nullptr && m_sock->state() != QAbstractSocket::ConnectedState)
    {
        qCCritical(logxfer) << m_name << "_transferSockToTx on invalid socket";
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

ssize_t SshTunnelDataConnector::_transferTxToSsh()
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
            char *emsg;
            int size;
            int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                /* Process next connection */
                return LIBSSH2_ERROR_EAGAIN;
            }
            qCCritical(logxfer) << m_name << "Error" << ret << "libssh2_channel_write" << QString(emsg);
            return ret;
        }
        if (len == 0)
        {
            qCWarning(logxfer) << m_name << "ERROR:  libssh2_channel_write return 0";
            return 0;
        }
        /* xfer OK */
        m_output += len;
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

ssize_t SshTunnelDataConnector::_transferSshToRx()
{
    ssize_t sshread = 0;

    if(m_rx_stop_ptr != nullptr)
    {
        qCWarning(logxfer) << "Buffer not empty, need to retry later";
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

            char *emsg;
            int size;
            int ret = libssh2_session_last_error(m_sshClient->session(), &emsg, &size, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                /* Process next connection */
                return LIBSSH2_ERROR_EAGAIN;
            }
            qCCritical(logxfer) << m_name << "Error" << ret << QString("libssh2_channel_read (%1 / %2)").arg(sshread).arg(BUFFER_SIZE) << QString(emsg);
            return ret;
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

ssize_t SshTunnelDataConnector::_transferRxToSock()
{
    ssize_t total = 0;

    /* If socket not ready, wait for socket connected */
    if(!m_sock || m_sock->state() != QAbstractSocket::ConnectedState)
    {
        qCWarning(logxfer) << m_name << "_transferRxToSock: Data on SSH when socket closed";
        return -1;
    }

    if(m_rx_stop_ptr == nullptr)
    {
        qCWarning(logxfer) <<  m_name << "Buffer empty";
        return 0;
    }

    DEBUGCH << "_transferRxToSock: libssh2_channel_read return " << (m_rx_stop_ptr - m_rx_start_ptr) << "bytes";

    while (m_rx_start_ptr < m_rx_stop_ptr)
    {
        ssize_t slen = m_sock->write(m_rx_start_ptr, m_rx_stop_ptr - m_rx_start_ptr);
        if (slen <= 0)
        {
            qCWarning(logxfer) << "ERROR : " << m_name << " local failed to write (" << slen << ")";
            return slen;
        }
        m_input += slen;
        m_rx_start_ptr += slen;
        total += slen;
        DEBUGCH << "_transferRxToSock: " << slen << "bytes written on socket";
    }

    /* Buffer is empty */
    m_rx_stop_ptr = nullptr;
    m_rx_start_ptr = nullptr;
    return total;
}

void SshTunnelDataConnector::sshDataReceived()
{
    m_data_to_rx = true;
}

bool SshTunnelDataConnector::process()
{
    DEBUGCH << "Process transfer RX(" << m_data_to_rx << ") TX(" << m_data_to_tx << ") BUFTX(" << (m_tx_stop_ptr - m_tx_start_ptr)  << ") BUFRX(" << (m_rx_stop_ptr - m_rx_start_ptr) << ")";
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
        return false;
    }
    if(m_rx_closed && (m_rx_start_ptr == nullptr))
    {
        DEBUGCH << "Send EOF to Socket";
        m_sock->disconnectFromHost();
        return false;
    }
    return true;
}

void SshTunnelDataConnector::close()
{
    if(m_sock && m_sock->state() == QAbstractSocket::ConnectedState)
    {
        m_sock->disconnectFromHost();
    }
}

bool SshTunnelDataConnector::isClosed()
{
    return m_tx_closed && m_rx_closed;
}
