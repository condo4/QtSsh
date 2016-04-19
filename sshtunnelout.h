#ifndef SSHTUNNELOUT_H
#define SSHTUNNELOUT_H

#include <QAbstractSocket>
#include "sshchannel.h"

class QTcpServer;
class QTcpSocket;
class SshClient;


class SshTunnelOut: public SshChannel
{
    Q_OBJECT

private:
    bool _opened;
    quint16 _port;
    QString _name;
    QTcpSocket *_tcpsocket;

public:
    explicit SshTunnelOut(SshClient *client, QTcpSocket *tcpSocket, QString port_identifier, quint16 port);
    void close(QString reason);

protected slots:
    void sshDataReceived();

private slots:
    void displayError(QAbstractSocket::SocketError error);
    void tcpDataReceived();
    void tcpDisconnected();


signals:
    void disconnected();
    void channelReady();
    
};

#endif // SSHTUNNELOUT_H
