#pragma once

#include <QObject>
#include <QTcpServer>
#include <QLoggingCategory>
#include "sshchannel.h"

#define BUFFER_SIZE (128*1024)

Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutconnection)
Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutconnectiontransfer)

class SshTunnelOutConnection : public SshChannel
{
    Q_OBJECT

public:
    explicit SshTunnelOutConnection(const QString &name, SshClient *client, QTcpServer &server, quint16 remotePort);
    virtual ~SshTunnelOutConnection() override;
    void close() override;







private:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
    QTcpSocket *m_sock;
    QTcpServer &m_server;
    quint16 m_port;

    bool m_dataWaitingOnSsh {false};
    bool m_dataWaitingOnSock {true}; /* When socket is connected, there is perhaps some waiting data */


    char m_tx_buffer[BUFFER_SIZE] {0,};
    char *m_tx_stop_ptr {nullptr};
    bool m_sshWaiting {false};
    bool m_disconnectedFromSsh {false};
    bool m_disconnectedFromSock {false};
    ssize_t m_writen {0};
    ssize_t m_readen {0};
    bool m_error {false};

    int _displaySshError(const QString &msg);

    /* Transfer function */
    ssize_t _transferSockToTx();
    ssize_t _transferTxToSsh();

    /* States Action */
    int _creating();
    int _running();
    int _closing();
    int _waitingClosed();
    int _freeing();

private slots:
    void _socketDisconnected();
    void _socketDataRecived();
    void _socketError();
    void _eventLoop(QString why);


public slots:
    void sshDataReceived() override;
};
