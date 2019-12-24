#include <QCoreApplication>
#include <signal.h>
#include <QTest>

#include "tester.h"



static void (*basicMessageHandler)(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString newMsg;
    newMsg = "\t" + QTime::currentTime().toString("hh:mm:ss:zzz") + ": " + msg;
    (*basicMessageHandler)(type, context, newMsg);
}


int main(int argc, char *argv[])
{
    //bool ret;
    basicMessageHandler = qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);
    /*
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);
    */

    Tester test("127.0.0.1", "lpctest", "lpctest");
    return QTest::qExec(&test, argc, argv);
}
