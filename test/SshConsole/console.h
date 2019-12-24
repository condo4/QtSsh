#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QTcpServer>

class Console : public QObject
{
    Q_OBJECT
    QTcpServer m_server;
    int m_instance {0};

public:
    explicit Console(QObject *parent = nullptr);

signals:

public slots:

private slots:
    void _newConnection();
};

#endif // CONSOLE_H
