#include "console.h"
#include "connection.h"

Console::Console(QObject *parent) : QObject(parent)
{
    QObject::connect(&m_server, &QTcpServer::newConnection, this, &Console::_newConnection);
    m_server.listen();
    qInfo() << "Listen on " << m_server.serverPort();
}

void Console::_newConnection()
{
    new Connection(QString("#%1").arg(m_instance++), m_server, this);
}
