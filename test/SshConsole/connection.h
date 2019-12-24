#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QTcpSocket>

class QTcpServer;
class SshClient;


class Connection : public QObject
{
    Q_OBJECT
    QScopedPointer<QTcpSocket> m_sock;
    SshClient* m_ssh {nullptr};
    QString m_name;

public:
    explicit Connection(QString name, QTcpServer &server, QObject *parent = nullptr);
    virtual ~Connection() override;

signals:

public slots:

private slots:
    void _input();
    void _sshConnected();
    void _sshDisconnected();

private:
    void _print(QString msg);
};

#endif // CONNECTION_H
