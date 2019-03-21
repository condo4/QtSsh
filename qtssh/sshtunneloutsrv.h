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
    QString                 m_identifier;
    quint16                 m_port;
    int                     m_count;

public:
    explicit SshTunnelOutSrv(SshClient * client, const QString &port_identifier, quint16 port);
    ~SshTunnelOutSrv();
    quint16 localPort();

signals:

public slots:
    void createConnection();
    void connectionDestroyed();
};
