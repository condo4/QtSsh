# QtSsh
QT             += core network

HEADERS += \
    $$PWD/qtssh/sshtunnelout.h \
    $$PWD/qtssh/sshtunnelin.h \
    $$PWD/qtssh/sshprocess.h \
    $$PWD/qtssh/sshchannel.h \
    $$PWD/qtssh/sshclient.h \
    $$PWD/qtssh/sshscpsend.h \
    $$PWD/qtssh/sshscpget.h \
    $$PWD/qtssh/sshsftp.h \
    $$PWD/qtssh/async_libssh2.h \
    $$PWD/qtssh/sshkey.h \
    $$PWD/qtssh/sshtunnelinconnection.h \
    $$PWD/qtssh/sshtunneloutconnection.h


SOURCES += \
    $$PWD/qtssh/sshtunnelout.cpp \
    $$PWD/qtssh/sshtunnelin.cpp \
    $$PWD/qtssh/sshprocess.cpp \
    $$PWD/qtssh/sshchannel.cpp \
    $$PWD/qtssh/sshclient.cpp \
    $$PWD/qtssh/sshscpsend.cpp \
    $$PWD/qtssh/sshscpget.cpp \
    $$PWD/qtssh/sshsftp.cpp \
    $$PWD/qtssh/sshkey.cpp \
    $$PWD/qtssh/sshtunnelinconnection.cpp \
    $$PWD/qtssh/sshtunneloutconnection.cpp

INCLUDEPATH += $$PWD/qtssh
