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
    SshClient               *m_sshclient {nullptr};
    quint16                 m_port {0};
    QWeakPointer<SshTunnelOut> m_myself;

public slots:
    bool isClosed();
    virtual void disconnectChannel() override;

private slots:
    void createConnection();

protected:
    virtual void close() override;
    explicit SshTunnelOut(SshClient * client, const QString &port_identifier, quint16 port);
    friend class SshClient;

protected slots:
    void _removeClosedConnection(SshTunnelOutConnection *ch);
    friend class SshTunnelOutConnection;

public:
    virtual ~SshTunnelOut();
    quint16 localPort() override;
    void setSharedPointer(QSharedPointer<SshTunnelOut> &ptr);

signals:
    void connectionClosed();
    void closed();
};
