#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QLoggingCategory>
#include "sshchannel.h"

#define BUFFER_SIZE 16384

Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutconnection)
Q_DECLARE_LOGGING_CATEGORY(logsshtunneloutconnectiontransfer)

class SshTunnelOutConnection : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        None,
        Error,
        Creating,
        Running,
        Freeing
    };
    Q_ENUM(ConnectionState)
    explicit SshTunnelOutConnection(const QString &name, SshClient *client, QTcpSocket *sock, quint16 remotePort, QObject *parent = nullptr);
    void disconnectFromHost();
    bool isClosed();

private:
    SshTunnelOut *m_parent;
    ConnectionState m_state {ConnectionState::None};
    LIBSSH2_CHANNEL *m_channel {nullptr};
    SshClient *m_client {nullptr};
    QTcpSocket *m_sock;
    quint16 m_port;
    QString m_name;
    char m_rx_buffer[BUFFER_SIZE] {0,};
    char m_tx_buffer[BUFFER_SIZE] {0,};
    char *m_tx_start_ptr {nullptr};
    char *m_rx_start_ptr {nullptr};
    char *m_tx_stop_ptr {nullptr};
    char *m_rx_stop_ptr {nullptr};
    bool _sshWaiting;

    int _displaySshError(const QString &msg);

    /* Transfer function */
    ssize_t _transferSshToRx();
    ssize_t _transferRxToSock();
    ssize_t _transferSockToTx();
    ssize_t _transferTxToSsh();

    /* States Action */
    int _creating();
    int _running();
    int _closing();
    int _waitingClosed();
    int _freeing();

private slots:
    void _socketDataReceived();
    void _socketDisconnected();
    void _socketDestroyed();
    void _socketError();

    void _sshDataReceived();
    void _sshDisconnected();
};
