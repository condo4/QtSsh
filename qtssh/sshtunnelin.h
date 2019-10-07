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

    void free();

public:
    virtual ~SshTunnelIn() override;
    quint16 localPort();
    quint16 remotePort();

    bool valid() const;
    void close() override;

private slots:
    void onLocalSocketDisconnected();
    void onLocalSocketConnected();
    void onLocalSocketError(QAbstractSocket::SocketError error);
    void onLocalSocketDataReceived();

    void sshDataReceived() override;

private:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};

};
