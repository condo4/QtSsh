#pragma once

#include <QObject>
#include "sshchannel.h"
#include <QTcpServer>

class SshTunnelOut;
class SshClient;

Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutsrv)

class SshTunnelOutSrv : public SshChannel
{
    Q_OBJECT

    QTcpServer              m_tcpserver;
    QList<QSharedPointer<SshTunnelOut>>    m_connections;
    SshClient               *m_sshclient;
    quint16                 m_port;
    int                     m_count;

protected:
    virtual void close() override;
    explicit SshTunnelOutSrv(SshClient * client, const QString &port_identifier, quint16 port);
    friend class SshClient;

public:
    quint16 localPort() override;

signals:

public slots:
    void createConnection();
    void connectionDestroyed();
};
