#ifndef SSHTUNNELDATACONNECTOR_H
#define SSHTUNNELDATACONNECTOR_H

#include <QObject>
#include <QLoggingCategory>
#include "sshchannel.h"
class QTcpSocket;

#define BUFFER_SIZE (128*1024)

Q_DECLARE_LOGGING_CATEGORY(logxfer)

class SshTunnelDataConnector : public QObject
{
    Q_OBJECT

    SshClient *m_sshClient  {nullptr};
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
    QTcpSocket *m_sock  {nullptr};
    QString m_name;

    char m_tx_buffer[BUFFER_SIZE] {0,};
    char *m_tx_start_ptr {nullptr};
    char *m_tx_stop_ptr {nullptr};
    bool m_tx_closed {false};
    bool m_data_to_tx {true};

    char m_rx_buffer[BUFFER_SIZE] {0,};
    char *m_rx_start_ptr {nullptr};
    char *m_rx_stop_ptr {nullptr};
    bool m_rx_closed {false};
    bool m_data_to_rx {false};

    /* Transfer function */
    ssize_t _transferSockToTx();
    ssize_t _transferTxToSsh();
    ssize_t _transferSshToRx();
    ssize_t _transferRxToSock();

    ssize_t m_output {0};
    ssize_t m_input {0};
    ssize_t m_input_real {0};

public slots:
    void sshDataReceived();
    bool process();
    void close();
    bool isClosed();

public:
    explicit SshTunnelDataConnector(SshClient *client, QString name, QObject *parent = nullptr);
    virtual ~SshTunnelDataConnector();
    void setChannel(LIBSSH2_CHANNEL *channel);
    void setSock(QTcpSocket *sock);

signals:
    void sendEvent();

private slots:
    void _socketDisconnected();
    void _socketDataRecived();
    void _socketError();
};

#endif // SSHTUNNELDATACONNECTOR_H
