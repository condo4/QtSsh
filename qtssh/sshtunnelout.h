#pragma once

#include <QObject>
#include "sshchannel.h"
#include <QTcpServer>

class SshClient;

Q_DECLARE_LOGGING_CATEGORY(logsshtunnelout)

class SshTunnelOut : public SshChannel
{
    Q_OBJECT

    struct Connection
    {
        LIBSSH2_CHANNEL *channel;
        QTcpSocket *sock;
        bool eof;
        bool closed;
    };

    QTcpServer              m_tcpserver;
    QList<struct Connection> m_connections;
    SshClient               *m_sshclient;
    quint16                 m_port;

    struct Connection &_connectionBySock(QTcpSocket *sock);
    int _closeChannel(struct Connection &c);
    int _freeChannel(struct Connection &c);

public slots:
    void sshDataReceived() override;

private slots:
    void _tcpDataReceived(QTcpSocket *sock = nullptr);
    void tcpDataReceived();
    void tcpDisconnected();
    void tcpError(QAbstractSocket::SocketError error);
    void createConnection();

protected:
    virtual void close() override;
    explicit SshTunnelOut(SshClient * client, const QString &port_identifier, quint16 port);
    friend class SshClient;

public:
    virtual ~SshTunnelOut();
    quint16 localPort() override;

signals:

};
