#include "sshtunnelout.h"
#include "sshclient.h"

Q_LOGGING_CATEGORY(logsshtunnelout, "ssh.tunnelout", QtWarningMsg)


SshTunnelOut::SshTunnelOut(SshClient *client, const QString &port_identifier, quint16 port):
    SshChannel(port_identifier, client),
    m_sshclient(client),
    m_port(port)
{
    m_tcpserver.listen(QHostAddress("127.0.0.1"), 0);
    QObject::connect(&m_tcpserver, &QTcpServer::newConnection, this, &SshTunnelOut::createConnection);
}

SshTunnelOut::~SshTunnelOut()
{
    qCDebug(logsshtunnelout) << m_name << "SshTunnelOut::~SshTunnelOut()";
    /* Disconnect all channels */
    while(m_connections.count())
    {
        Connection &ch = m_connections.last();
        if(ch.sock->state() == QTcpSocket::UnconnectedState && ch.channel == nullptr)
        {
            m_connections.pop_back();
            ch.sock->disconnect();
            ch.sock->deleteLater();
            continue;
        }
        else if(ch.sock->state() == QTcpSocket::UnconnectedState && ch.channel != nullptr)
        {
            _closeChannel(ch);
            _freeChannel(ch);
            continue;
        }
        qCDebug(logsshtunnelout) << m_name << "SshTunnelOut::~SshTunnelOut() " << m_connections.count() << ": " << ch.sock->localPort();
        ch.sock->disconnectFromHost();
    }

    qCDebug(logsshtunnelout) << m_name << "SshTunnelOut::~SshTunnelOut() OK";
}

void SshTunnelOut::close()
{
    qCDebug(logsshtunnelout) << m_name << "SshTunnelOut::close()";
    QList<Connection>::iterator iter = m_connections.begin();
    while(iter != m_connections.end())
    {
        /* Do Job for channel */
        qCDebug(logsshtunnelout) << m_name << "SshTunnelOut::disconnect socket " << iter->sock->localPort();
        iter->sock->disconnectFromHost();
        ++iter;
    }
    m_tcpserver.close();
}

void SshTunnelOut::createConnection()
{
    if(!m_sshclient->channelReady())
    {
        qDebug() << "WARNING : SshTunnelOut cannot open channel before connected()";
        return;
    }

    QTcpSocket *sock = m_tcpserver.nextPendingConnection();
    if(!sock) return;

    LIBSSH2_CHANNEL *channel = nullptr;
    while(channel == nullptr)
    {
        int ret;

        channel = qssh2_channel_direct_tcpip(m_sshclient->session(),  "127.0.0.1", m_port);
        if(channel == nullptr)
        {
            ret = libssh2_session_last_error(m_sshclient->session(), nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }

    if(channel == nullptr)
    {
        int ret = qssh2_session_last_error(m_sshclient->session(), nullptr, nullptr, 0);
        qCCritical(logsshtunnelout) << "Error during channel creation, code:" << ret <<",for port" << m_port;
        return;
    }


    QObject::connect(sock, &QTcpSocket::readyRead, this, &SshTunnelOut::tcpDataReceived);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &SshTunnelOut::tcpDisconnected);
    QObject::connect(sock, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(tcpError(QAbstractSocket::SocketError)));
    QObject::connect(m_sshClient, &SshClient::sshDataReceived, this, &SshTunnelOut::sshDataReceived, Qt::QueuedConnection);

    struct Connection ch;
    ch.channel = channel;
    ch.eof = false;
    ch.closed = false;
    ch.sock = sock;
    m_connections.push_back(ch);

    qCDebug(logsshtunnelout) << "SshTunnelOut::createConnection() " << m_name << " : " << sock->localPort() << " OK";
    _tcpDataReceived(sock);
}

quint16 SshTunnelOut::localPort()
{
    return m_tcpserver.serverPort();
}

void SshTunnelOut::_tcpDataReceived(QTcpSocket *sock)
{
    if(sock == nullptr)
    {
        qCWarning(logsshtunnelout) << "_tcpDataReceived socket error";
        return;
    }

    Connection &ch = _connectionBySock(sock);
    if(ch.channel == nullptr)
    {
        qCWarning(logsshtunnelout) << "tcpDataReceived channel error";
        return;
    }

    /* Do job: Copy data from socket to SSH channel */
    QByteArray dataSocket(16384, 0);
    qint64 len = 0;
    do
    {
        ssize_t wr = 0;
        ssize_t i = 0;
        /* Read data from local socket */
        len = sock->read(dataSocket.data(), dataSocket.length());
        if (-EAGAIN == len)
        {
            qCDebug(logsshtunnelout) << m_name << "tcpDataReceived() EAGAIN";
            break;
        }
        if (len < 0)
        {
            qCWarning(logsshtunnelout) <<  "ERROR : " << m_name << " local failed to read (" << len << ")";
            break;
        }

        if(len > 0 )
        {
            do
            {
                i = qssh2_channel_write(
                            ch.channel,
                            dataSocket.mid(static_cast<int>(wr)).constData(),
                            static_cast<size_t>(len-wr));
                if (i < 0)
                {
                    qCWarning(logsshtunnelout) << "ERROR : " << m_name << " remote failed to write (" << i << ")";
                    return;
                }
                if (i == 0)
                {
                    qCWarning(logsshtunnelout) << "ERROR : " << m_name << " qssh2_channel_write return 0";
                }
                wr += i;
            } while( wr != len);
        }
    }
    while(len > 0);
}

void SshTunnelOut::tcpDataReceived()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket *>(sender());
    if(sock == nullptr)
    {
        qCWarning(logsshtunnelout) << m_name << "tcpDataReceived sender error";
        return;
    }
    _tcpDataReceived(sock);
}

void SshTunnelOut::tcpDisconnected()
{
    /* Identify client socket */
    QTcpSocket *sock = qobject_cast<QTcpSocket *>(sender());
    if(sock == nullptr)
    {
        qCWarning(logsshtunnelout) << "tcpDataReceived sender error";
        return;
    }
    qCDebug(logsshtunnelout) << m_name << "tcpDisconnected:" << sock->localPort();

    Connection &ch = _connectionBySock(sock);
    if(ch.channel == nullptr)
    {
        qCWarning(logsshtunnelout) << "tcpDataReceived channel error";
        return;
    }

    _closeChannel(ch);
    _freeChannel(ch);
}

void SshTunnelOut::tcpError(QAbstractSocket::SocketError error)
{
    qCWarning(logsshtunnelout) << m_name << "socket error=" << error;
}

SshTunnelOut::Connection &SshTunnelOut::_connectionBySock(QTcpSocket *sock)
{
    for(Connection &con: m_connections)
    {
        if(con.sock == sock)
            return con;
    }
    return m_connections[0];
}

int SshTunnelOut::_closeChannel(Connection &channel)
{
    if(channel.closed)
        return 1;

    if(!channel.eof)
    {
        qssh2_channel_send_eof(channel.channel);
        channel.eof = true;
    }

    if(channel.closed || channel.channel == nullptr)
        return 1;

    qCDebug(logsshtunnelout) << m_name << "_closeChannel";
    /* Close channel */

    int ret = qssh2_channel_close(channel.channel);
    if(ret)
    {
        qCWarning(logsshtunnelout) << "Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
        return ret;
    }

    if(channel.closed || channel.channel == nullptr)
        return 1;

    ret = qssh2_channel_wait_closed(channel.channel);
    if(ret)
    {
        qCWarning(logsshtunnelout) << "Failed to channel_wait_closed: " << ret;
        return ret;
    }
    channel.closed = true;
    return 0;
}

int SshTunnelOut::_freeChannel(Connection &channel)
{
    if(channel.channel == nullptr)
        return 1;
    qCDebug(logsshtunnelout)  << m_name << "_freeChannel " << channel.channel;
    int ret = qssh2_channel_free(channel.channel);
    if(ret)
    {
        qCWarning(logsshtunnelout)  << "Failed to channel_free";
        return ret;
    }
    channel.channel = nullptr;
    return 0;
}

void SshTunnelOut::sshDataReceived()
{
    ssize_t len = 0;
    QByteArray dataSsh(16384, 0);
    QList<Connection>::iterator iter = m_connections.begin();
    while(iter != m_connections.end())
    {
        /* Do Job for channel */
        if(iter->channel == nullptr)
            continue;

        do
        {
            ssize_t wr = 0;
            qint64 i;
            /* Read data from SSH */
            /*
             * In this case, we must not used qssh2_channel_read
             * beacause we don't need to ProcessEvent to be locked
             * We can return, we will be recall at the next sshDataReceived
             */
            len = static_cast<ssize_t>(libssh2_channel_read(iter->channel, dataSsh.data(), static_cast<unsigned int>(dataSsh.size())));
            if(len == LIBSSH2_ERROR_EAGAIN)
            {
                return;
            }
            if (len < 0)
            {
                qCWarning(logsshtunnelout) <<  "ERROR : " << m_name << " remote failed to read (" << len << " / " << dataSsh.size() << ")";
                return;
            }

            /* Write data into output local socket */
            if(iter->sock != nullptr)
            {
                while (wr < len)
                {
                    i = iter->sock->write(dataSsh.mid(static_cast<int>(wr)).constData(), static_cast<int>(len-wr));
                    if (i <= 0)
                    {
                        qCWarning(logsshtunnelout) << "ERROR : " << m_name << " local failed to write (" << i << ")";
                        return;
                    }
                    wr += i;
                }
            }
        }
        while(len != 0);

        if (libssh2_channel_eof(iter->channel))
        {
            iter->sock->disconnectFromHost();
            iter->eof = true;
            qCDebug(logsshtunnelout) << m_name << "Disconnected from ssh";
        }
        ++iter;
    }
}
