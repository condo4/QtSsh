#ifndef SSHTUNNELIN_H
#define SSHTUNNELIN_H

#include "sshserviceport.h"
#include "sshchannel.h"
#include <QAbstractSocket>
class QTcpSocket;

class SshTunnelIn : public SshChannel, public SshServicePort
{
    Q_OBJECT

    enum SshTunnelInState {
        TunnelError = 0,
        TunnelListenTcpServer = 1,
        TunnelAcceptChannel = 2,
        TunnelReadyRead = 3,
        TunnelErrorNoRetry = 4
    };
private:
    quint16 _localTcpPort;
    LIBSSH2_LISTENER *_sshListener;
    SshTunnelInState _currentState;
    quint16 _port;
    QString _name;
    QTcpSocket *_tcpsocket;

public:
    explicit SshTunnelIn(SshClient * client, QString port_identifier, quint16 port, quint16 bind);
    quint16 localPort();

private slots:
    void onLocalSocketDisconnected();
    void onLocalSocketError(QAbstractSocket::SocketError error);
    void onLocalSocketDataReceived();

    virtual void sshDataReceived();
    void readSshData();
};

#endif // SSHTUNNELIN_H
