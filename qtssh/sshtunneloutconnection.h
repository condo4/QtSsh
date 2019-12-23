#pragma once

#include <QObject>
#include <QTcpServer>
#include <QLoggingCategory>
#include "sshchannel.h"
#include "sshtunneldataconnector.h"

#define BUFFER_SIZE (128*1024)

Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutconnection)
Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutconnectiontransfer)

class SshTunnelOutConnection : public SshChannel
{
    Q_OBJECT

public:
    explicit SshTunnelOutConnection(const QString &name, SshClient *client, QTcpServer &server, quint16 remotePort, QString target = "127.0.0.1");
    virtual ~SshTunnelOutConnection() override;
    void close() override;

private:
    SshTunnelDataConnector *m_connector {nullptr};
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
    QTcpSocket *m_sock;
    QTcpServer &m_server;
    quint16 m_port;
    QString m_target;
    bool m_error {false};

private slots:
    void _eventLoop();


public slots:
    void sshDataReceived() override;

signals:
    void sendEvent();
};
