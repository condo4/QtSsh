# QtSsh
QT             += core network

HEADERS += \
    $$PWD/qtssh/sshsftpcommandfileinfo.h \
    $$PWD/qtssh/sshsftpcommandget.h \
    $$PWD/qtssh/sshsftpcommandmkdir.h \
    $$PWD/qtssh/sshsftpcommandreaddir.h \
    $$PWD/qtssh/sshsftpcommandsend.h \
    $$PWD/qtssh/sshsftpcommandunlink.h \
    $$PWD/qtssh/sshtunnelout.h \
    $$PWD/qtssh/sshtunnelin.h \
    $$PWD/qtssh/sshprocess.h \
    $$PWD/qtssh/sshchannel.h \
    $$PWD/qtssh/sshclient.h \
    $$PWD/qtssh/sshscpsend.h \
    $$PWD/qtssh/sshscpget.h \
    $$PWD/qtssh/sshsftp.h \
    $$PWD/qtssh/sshsftpcommand.h \
    $$PWD/qtssh/sshkey.h \
    $$PWD/qtssh/sshtunnelinconnection.h \
    $$PWD/qtssh/sshtunneloutconnection.h \
    $$PWD/qtssh/sshtunneldataconnector.h


SOURCES += \
    $$PWD/qtssh/sshsftpcommandfileinfo.cpp \
    $$PWD/qtssh/sshsftpcommandget.cpp \
    $$PWD/qtssh/sshsftpcommandmkdir.cpp \
    $$PWD/qtssh/sshsftpcommandreaddir.cpp \
    $$PWD/qtssh/sshsftpcommandsend.cpp \
    $$PWD/qtssh/sshsftpcommandunlink.cpp \
    $$PWD/qtssh/sshtunnelout.cpp \
    $$PWD/qtssh/sshtunnelin.cpp \
    $$PWD/qtssh/sshprocess.cpp \
    $$PWD/qtssh/sshchannel.cpp \
    $$PWD/qtssh/sshclient.cpp \
    $$PWD/qtssh/sshscpsend.cpp \
    $$PWD/qtssh/sshscpget.cpp \
    $$PWD/qtssh/sshsftp.cpp \
    $$PWD/qtssh/sshsftpcommand.cpp \
    $$PWD/qtssh/sshkey.cpp \
    $$PWD/qtssh/sshtunnelinconnection.cpp \
    $$PWD/qtssh/sshtunneloutconnection.cpp \
    $$PWD/qtssh/sshtunneldataconnector.cpp

INCLUDEPATH += $$PWD/qtssh
