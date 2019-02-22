#pragma once

#include <QAbstractSocket>
#include <QMutex>
#include "sshchannel.h"

class QTcpServer;
class QTcpSocket;
class SshClient;

Q_DECLARE_LOGGING_CATEGORY(logsshtunnelout)

class SshTunnelOut: public QObject
{
    Q_OBJECT

private:
    quint16 m_port;
    QString m_name;
    QTcpSocket *m_tcpsocket {nullptr};
    SshClient *m_client {nullptr};
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
    QByteArray m_dataSsh;
    QByteArray m_dataSocket;
    QMutex m_mutex;

public:
    explicit SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, const QString &port_identifier, quint16 port, QObject *parent);
    virtual ~SshTunnelOut();
    void disconectFromHost();
    QString name() const;

public slots:
    void sshDataReceived();

private slots:
    void displayError(QAbstractSocket::SocketError error);
    void tcpDataReceived();
    void tcpDisconnected();


signals:
    void disconnectedFromTcp();
    void disconnectedFromSsh();
    void channelReady();
};
