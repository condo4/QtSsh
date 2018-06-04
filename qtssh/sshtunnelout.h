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
    bool _opened;
    quint16 _port;
    QString _name;
    QTcpSocket *_tcpsocket;
    SshClient *_client;
    LIBSSH2_CHANNEL *_sshChannel;
    QByteArray _dataSsh;
    QByteArray _dataSocket;

    unsigned int _callDepth;

    void _stopChannel();
    void _stopSocket();

public:
    explicit SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, QString port_identifier, quint16 port, QObject *parent);
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
