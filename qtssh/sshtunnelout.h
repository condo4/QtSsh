#pragma once

#include <QObject>
#include "sshchannel.h"
#include "sshtunneloutconnection.h"
#include <QTcpServer>

Q_DECLARE_LOGGING_CATEGORY(logsshtunnelout)

class SshTunnelOut : public SshChannel
{
    Q_OBJECT

protected:
    explicit SshTunnelOut(const QString &name, SshClient * client);
    friend class SshClient;

public:
    virtual ~SshTunnelOut() override;
    void close() override;
    quint16 localPort();

public slots:
    void listen(quint16 port);
    void sshDataReceived() override;

private:
    QTcpServer              m_tcpserver;
    quint16                 m_port {0};

private slots:
    void _createConnection();
};
