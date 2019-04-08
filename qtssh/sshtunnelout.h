#pragma once

#include <QAbstractSocket>
#include <QMutex>
#include "sshchannel.h"

class QTcpServer;
class QTcpSocket;
class SshClient;

Q_DECLARE_LOGGING_CATEGORY(logsshtunnelout)

class SshTunnelOut: public SshChannel
{
    Q_OBJECT

private:
    quint16 m_port;
    QTcpSocket *m_tcpsocket {nullptr};
    SshClient *m_client {nullptr};
    QByteArray m_dataSsh;
    QByteArray m_dataSocket;
    int m_retryChannelCreation;
    QMutex m_mutextSsh;
    QMutex m_mutextTcp;

protected:
    explicit SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, const QString &port_identifier, quint16 port);
    friend class SshTunnelOutSrv;

public:
    virtual ~SshTunnelOut();
    QString name() const;

protected:
    void close() override;

public slots:
    void sshDataReceived() override;

private slots:
    void displayError(QAbstractSocket::SocketError error);
    void tcpDataReceived();
    void tcpDisconnected();
    void _init_channel();

signals:
    void disconnectedFromTcp();
    void disconnectedFromSsh();
    void channelReady();
};
