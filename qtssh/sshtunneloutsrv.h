#pragma once

#include <QObject>
#include "sshchannel.h"
#include <QTcpServer>

class SshTunnelOut;
class SshClient;

class SshTunnelOutSrv : public SshChannel
{
    Q_OBJECT

    QTcpServer              _tcpserver;
    QList<SshTunnelOut*>    _connections;
    SshClient               *_sshclient;
    QString                 _identifier;
    quint16                 _port;
    int                     _count;

public:
    explicit SshTunnelOutSrv(SshClient * client, QString port_identifier, quint16 port);
    ~SshTunnelOutSrv();
    quint16 localPort();

signals:

public slots:
    void createConnection();
    void connectionDisconnected();
};
