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
    int m_localTcpPort;
    int m_remoteTcpPort;
    LIBSSH2_LISTENER *m_sshListener;
    quint16 m_port;
    QScopedPointer<QTcpSocket, QScopedPointerDeleteLater> m_tcpsocket;
    bool m_valid;
    bool m_workinprogress;
    bool m_needToDisconnect;
    bool m_needToSendEOF;
    QByteArray m_tcpBuffer;
    QByteArray m_sshBuffer;

protected:
    explicit SshTunnelIn(SshClient * client, const QString &portIdentifier, quint16 localport, quint16 remoteport, QString host = "localhost");
    friend class SshClient;

public:
    virtual ~SshTunnelIn();
    quint16 localPort() override;
    quint16 remotePort();

    bool valid() const;

private slots:
    void onLocalSocketDisconnected();
    void onLocalSocketConnected();
    void onLocalSocketError(QAbstractSocket::SocketError error);
    void onLocalSocketDataReceived();

    virtual void sshDataReceived() override;

protected:
    virtual void close() override;
};
