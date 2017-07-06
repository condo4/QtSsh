#ifndef SSHTUNNELOUTSRV_H
#define SSHTUNNELOUTSRV_H

#include <QObject>
#include "sshchannel.h"
#include <QTcpServer>

class SshTunnelOut;
class SshClient;

class SshTunnelOutSrv : public SshChannel
{
    Q_OBJECT

    QTcpServer              *_tcpserver;
    QList<SshTunnelOut*>    _connections;
    SshClient               *_sshclient;
    QString                 _identifier;
    quint16                 _port;

public:
    explicit SshTunnelOutSrv(SshClient * client, QString port_identifier, quint16 port);
    ~SshTunnelOutSrv();
    quint16 localPort();
    void sshDataReceived();

signals:

public slots:
    void createConnection();
    void connectionDisconnected();
};

#endif // SSHTUNNELOUTSRV_H
