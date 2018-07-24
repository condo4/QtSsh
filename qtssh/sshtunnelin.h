#pragma once

#include "sshchannel.h"
#include <QAbstractSocket>
#include <QLoggingCategory>

class QTcpSocket;

Q_DECLARE_LOGGING_CATEGORY(logsshtunnelin)

class SshTunnelIn : public SshChannel
{
    Q_OBJECT

private:
    quint16 m_localTcpPort;
    LIBSSH2_LISTENER *m_sshListener;
    quint16 m_port;
    QString m_name;
    QTcpSocket *m_tcpsocket;
    bool m_valid;

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
