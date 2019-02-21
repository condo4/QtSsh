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
    bool m_workinprogress;
    bool m_needToDisconnect;
    bool m_needToSendEOF;

public:
    explicit SshTunnelIn(SshClient * client, const QString &portIdentifier, quint16 port, quint16 bind, QString host = "localhost");
    virtual ~SshTunnelIn();
    quint16 localPort();

    bool valid() const;

private slots:
    void onLocalSocketDisconnected();
    void onLocalSocketConnected();
    void onLocalSocketError(QAbstractSocket::SocketError error);
    void onLocalSocketDataReceived();

    virtual void sshDataReceived();
};
