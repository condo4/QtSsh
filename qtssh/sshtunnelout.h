#pragma once

#include <QAbstractSocket>
#include "sshchannel.h"

class QTcpServer;
class QTcpSocket;
class SshClient;


class SshTunnelOut: public QObject
{
    Q_OBJECT

private:
    bool m_opened;
    quint16 m_port;
    QString m_name;
    QTcpSocket *m_tcpsocket;
    SshClient *m_client;
    LIBSSH2_CHANNEL *m_sshChannel;
    QByteArray m_dataSsh;
    QByteArray m_dataSocket;

    unsigned int m_callDepth;

    void _stopChannel();
    void _stopSocket();

public:
    explicit SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, const QString &port_identifier, quint16 port, QObject *parent);
    virtual ~SshTunnelOut();
    void close(QString reason);
    bool ready() const;
    QString name() const;

public slots:
    void sshDataReceived();

private slots:
    void displayError(QAbstractSocket::SocketError error);
    void tcpDataReceived();
    void tcpDisconnected();


signals:
    void disconnected();
    void channelReady();
    
};
