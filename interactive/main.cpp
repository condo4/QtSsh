#include <QCoreApplication>
#include <QTimer>
#include "console.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Console console;

    return app.exec();
}
