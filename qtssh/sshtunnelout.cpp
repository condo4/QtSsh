#include "sshtunnelout.h"
#include "sshclient.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>
#include <cerrno>

Q_LOGGING_CATEGORY(logsshtunnelout, "ssh.tunnelout", QtWarningMsg)

SshTunnelOut::SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, const QString &port_identifier, quint16 port, QObject *parent):
    QObject(parent),
    m_port(port),
    m_name(port_identifier),
    m_tcpsocket(tcpSocket),
    m_client(client),
    m_dataSsh(16384, 0),
    m_dataSocket(16384, 0)
{
    m_sshChannel = qssh2_channel_direct_tcpip(m_client->session(), "127.0.0.1", m_port);
    if(m_sshChannel == nullptr)
    {
        int ret = qssh2_session_last_error(m_client->session(), nullptr, nullptr, 0);
        if(ret != LIBSSH2_ERROR_CHANNEL_FAILURE)
        {
            qDebug() << "ERROR: Can't connect direct tcpip " << ret << " for port " << m_port;
        }
        else
        {
            qCDebug(logsshtunnelout) << "Can't connect direct tcpip " << ret << " for port " << m_port;
        }
        return;
    }

    QObject::connect(m_client,    &SshClient::sshDataReceived, this, &SshTunnelOut::sshDataReceived);
    QObject::connect(m_tcpsocket, &QTcpSocket::readyRead,      this, &SshTunnelOut::tcpDataReceived);
    QObject::connect(m_tcpsocket, &QTcpSocket::disconnected,   this, &SshTunnelOut::tcpDisconnected);
    QObject::connect(m_tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),   this, SLOT(displayError(QAbstractSocket::SocketError)));

    qCDebug(logsshtunnelout) << "SshTunnelOut::SshTunnelOut() OK " << m_name;
    tcpDataReceived();
}

SshTunnelOut::~SshTunnelOut()
{
    qCDebug(logsshtunnelout) << "~SshTunnelOut() " << m_name << " " << this;
    if(m_tcpsocket && (m_tcpsocket->state() == QAbstractSocket::ConnectedState))
    {
        m_tcpsocket->disconnectFromHost();
    }

    if (m_sshChannel == nullptr)
        return;

    int ret = qssh2_channel_close(m_sshChannel);
    if(ret)
    {
        qCDebug(logsshtunnelout) << "Failed to channel_close: LIBSSH2_ERROR_SOCKET_SEND";
        return;
    }

    ret = qssh2_channel_wait_closed(m_sshChannel);
    if(ret)
    {
        qDebug(logsshtunnelout) << "Failed to channel_wait_closed";
        return;
    }

    ret = qssh2_channel_free(m_sshChannel);
    if(ret)
    {
        qCDebug(logsshtunnelout) << "Failed to channel_free";
        return;
    }
}

void SshTunnelOut::disconectFromHost()
{
    if(m_tcpsocket && (m_tcpsocket->state() == QAbstractSocket::ConnectedState))
    {
        m_tcpsocket->disconnectFromHost();
    }
}

QString SshTunnelOut::name() const
{
    return m_name;
}

void SshTunnelOut::sshDataReceived()
{
    if(!m_mutex.tryLock())
        return;

    ssize_t len = 0,wr = 0;
    qint64 i;

    do
    {
        /* Read data from SSH */
        /*
         * In this case, we must not used qssh2_channel_read
         * beacause we don't need to ProcessEvent to be locked
         * We can return, we will be recall at the next sshDataReceived
         */
        len = static_cast<ssize_t>(libssh2_channel_read(m_sshChannel, m_dataSsh.data(), static_cast<unsigned int>(m_dataSsh.size())));
        if(len == LIBSSH2_ERROR_EAGAIN)
        {
            m_mutex.unlock();
            return;
        }
        if (len < 0)
        {
            qCWarning(logsshtunnelout) <<  "ERROR : " << m_name << " remote failed to read (" << len << " / " << m_dataSsh.size() << ")";
            m_mutex.unlock();
            return;
        }

        /* Write data into output local socket */
        wr = 0;
        if(m_tcpsocket != nullptr)
        {
            while (wr < len)
            {
                i = m_tcpsocket->write( m_dataSsh.mid(static_cast<int>(wr), static_cast<int>(len)));
                if (i <= 0)
                {
                    qCWarning(logsshtunnelout) << "ERROR : " << m_name << " local failed to write (" << i << ")";
                    m_mutex.unlock();
                    return;
                }
                wr += i;
            }
        }
    }
    while(len == m_dataSsh.size());

    if (libssh2_channel_eof(m_sshChannel))
    {
        if(m_tcpsocket)
            m_tcpsocket->disconnectFromHost();
        qCDebug(logsshtunnelout) << "Disconnected from ssh";
        emit disconnectedFromSsh();
    }
    m_mutex.unlock();
}

void SshTunnelOut::tcpDataReceived()
{
    if(!m_mutex.tryLock())
        return;
    qint64 len = 0;
    ssize_t wr = 0;
    ssize_t i = 0;

    if (m_tcpsocket == nullptr || m_sshChannel == nullptr)
    {
        qCWarning(logsshtunnelout) <<  "ERROR : SshTunnelOut(" << m_name << ") : received TCP data but not seems to be a valid Tcp socket or channel not ready";
        m_mutex.unlock();
        return;
    }

    do
    {
        /* Read data from local socket */
        len = m_tcpsocket->read(m_dataSocket.data(), m_dataSocket.size());
        if (-EAGAIN == len)
        {
            qCDebug(logsshtunnelout) << m_name << "tcpDataReceived() EAGAIN";
            break;
        }

        if (len < 0)
        {
            qCWarning(logsshtunnelout) <<  "ERROR : " << m_name << " local failed to read (" << len << ")";
            m_mutex.unlock();
            return;
        }

        if(len > 0)
        {
            do
            {
                i = qssh2_channel_write(m_sshChannel, m_dataSocket.data(), static_cast<size_t>(len));
                if (i < 0)
                {
                    qCWarning(logsshtunnelout) << "ERROR : " << m_name << " remote failed to write (" << i << ")";
                    m_mutex.unlock();
                    return;
                }
                if (i == 0)
                {
                    qCWarning(logsshtunnelout) << "ERROR : " << m_name << " qssh2_channel_write return 0";
                }
                wr += i;
            } while(i > 0 && wr < len);
        }
    }
    while(len > 0);
    m_mutex.unlock();
}

void SshTunnelOut::tcpDisconnected()
{
    qCDebug(logsshtunnelout) << "Disconnected from socket";
    m_tcpsocket->deleteLater();
    m_tcpsocket = nullptr;
    qssh2_channel_send_eof(m_sshChannel);
    emit disconnectedFromTcp();
}

void SshTunnelOut::displayError(QAbstractSocket::SocketError error)
{
    qCWarning(logsshtunnelout) << "ERROR : SshTunnelOut(" << m_name << ") : redirection socket error=" << error;
}

