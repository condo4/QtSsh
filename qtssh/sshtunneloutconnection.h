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
    explicit SshTunnelOutConnection(const QString &name, SshClient *client, QTcpServer &server, quint16 remotePort, QString target = "127.0.0.1");
    virtual ~SshTunnelOutConnection() override;
    void close() override;

private:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
    QTcpSocket *m_sock;
    QTcpServer &m_server;
    quint16 m_port;
    QString m_target;

    char m_tx_buffer[BUFFER_SIZE] {0,};
    char *m_tx_start_ptr {nullptr};
    char *m_tx_stop_ptr {nullptr};
    bool m_tx_closed {false};
    bool m_data_to_tx {false};

    char m_rx_buffer[BUFFER_SIZE] {0,};
    char *m_rx_start_ptr {nullptr};
    char *m_rx_stop_ptr {nullptr};
    bool m_rx_closed {false};
    bool m_data_to_rx {false};


    bool m_disconnectedFromSsh {false};
    bool m_disconnectedFromSock {false};
    ssize_t m_writen {0};
    ssize_t m_readen {0};
    bool m_error {false};

    int _displaySshError(const QString &msg);

    /* Transfer function */
    ssize_t _transferSockToTx();
    ssize_t _transferTxToSsh();
    ssize_t _transferSshToRx();
    ssize_t _transferRxToSock();


private slots:
    void _socketDisconnected();
    void _socketDataRecived();
    void _socketError();
    void _eventLoop();


public slots:
    void sshDataReceived() override;

signals:
    void sendEvent();
};
