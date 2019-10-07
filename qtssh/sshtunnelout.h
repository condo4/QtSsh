#pragma once

#include <QObject>
#include "sshchannel.h"
#include "sshtunneloutconnection.h"
#include <QTcpServer>

class SshClient;

Q_DECLARE_LOGGING_CATEGORY(logsshtunnelout)

#define SOCKET_WRITE_ERROR (-1001)

class SshTunnelOut : public SshChannel
{
    Q_OBJECT

public:
    enum ConnectionEventOrigin {
        FromSocket,
        FromSsh
    };
    Q_ENUM(ConnectionEventOrigin)

private:
    QTcpServer              m_tcpserver;
    QList<SshTunnelOutConnection *> m_connections;
    quint16                 m_port {0};
    QWeakPointer<SshTunnelOut> m_myself;

public slots:
    bool isClosed();
    void listen(quint16 port);

private slots:
    void createConnection();

protected:
    explicit SshTunnelOut(const QString &name, SshClient * client);
    friend class SshClient;

protected slots:
    void _removeClosedConnection(SshTunnelOutConnection *ch);
    friend class SshTunnelOutConnection;

public:
    virtual ~SshTunnelOut() override;
    quint16 localPort();
    void setSharedPointer(QSharedPointer<SshTunnelOut> &ptr);
    void close() override;

signals:
    void connectionClosed();
    void closed();
};
