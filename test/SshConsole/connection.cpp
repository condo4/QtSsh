#include "connection.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QCoreApplication>
#include <sshclient.h>
#include <sshtunnelout.h>

Connection::Connection(QString name, QTcpServer &server, QObject *parent)
    : QObject(parent)
    , m_sock(server.nextPendingConnection())
    , m_name(name)
{
    QObject::connect(m_sock.data(), &QTcpSocket::readyRead, this, &Connection::_input);
    _print("?: connect <login>@<host>:<password>");
}

Connection::~Connection()
{
    delete m_ssh;
}

void Connection::_print(QString msg)
{
    m_sock->write(m_name.toUtf8() + "> ");
    m_sock->write(msg.toUtf8());
    if(!msg.endsWith("\n"))
    {
        m_sock->write("\n");
    }
}

void Connection::_input()
{
    QByteArray data = m_sock->readAll().trimmed();
    if(data.startsWith("connect"))
    {
        auto t = data.split(' ');
        if(t.length() != 2)
        {
            _print("syntax error: connect <login>@<host>:<password>");
            return;
        }
        auto t1 = t[1].split('@');
        if(t1.length() != 2)
        {
            _print("syntax error: connect <login>@<host>:<password>");
            return;
        }
        QString login = t1[0];
        auto t2 = t1[1].split(':');
        if(t2.length() != 2)
        {
            _print("syntax error: connect <login>@<host>:<password>");
            return;
        }
        QString hostname = t2[0];
        QString password = t2[1];
        m_ssh = new SshClient(m_name, this);
        QObject::connect(m_ssh, &SshClient::sshReady, this, &Connection::_sshConnected);
        QObject::connect(m_ssh, &SshClient::sshDisconnected, this, &Connection::_sshDisconnected);
        m_ssh->setPassphrase(password);
        m_ssh->connectToHost(login, hostname);
    }
    if(data.startsWith("disconnect"))
    {
        _print("Ask disconnection");
        m_ssh->disconnectFromHost();
    }
    if(data.startsWith("direct"))
    {
        auto t = data.split(' ');
        if(t.length() != 2)
        {
            _print("?: direct:<port>");
            return;
        }
        if(m_ssh)
        {
            SshTunnelOut *out = m_ssh->getChannel<SshTunnelOut>(t[1]);
            out->listen(t[1].toUShort());

            _print(QString("Direct tunnel for %1 on port %2").arg(t[1].toUShort()).arg(out->localPort()));
        }
    }
    if(data.startsWith("quit"))
    {
        QCoreApplication::quit();
    }
    if(data.startsWith("exit"))
    {
        _print("Bye bye");
        m_sock->close();
        deleteLater();
    }
}

void Connection::_sshConnected()
{
    _print("connected");
    _print("?: disconnect");
    _print("?: direct <port>");
}

void Connection::_sshDisconnected()
{
    _print("disconnected");
    delete m_ssh;
    m_ssh = nullptr;
}
