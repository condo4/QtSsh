#ifndef SSHTUNNELIN_H
#define SSHTUNNELIN_H

#include "sshchannel.h"
#include <QAbstractSocket>
class QTcpSocket;

class SshTunnelIn : public SshChannel
{
    Q_OBJECT

private:
    quint16 _localTcpPort;
    LIBSSH2_LISTENER *_sshListener;
    quint16 _port;
    QString _name;
    QTcpSocket *_tcpsocket;
    bool _valid;

public:
    explicit SshTunnelIn(SshClient * client, QString port_identifier, quint16 port, quint16 bind);
    virtual ~SshTunnelIn();
    quint16 localPort();

    bool valid() const;

private slots:
    void onLocalSocketDisconnected();
    void onLocalSocketError(QAbstractSocket::SocketError error);
    void onLocalSocketDataReceived();

    virtual void sshDataReceived();
};

#endif // SSHTUNNELIN_H
