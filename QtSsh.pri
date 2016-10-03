# QtSsh
HEADERS += \
    $$PWD/private/sshtunnelout.h \
    $$PWD/private/sshtunnelin.h \
    $$PWD/private/sshprocess.h \
    $$PWD/private/sshchannel.h \
    $$PWD/sshclient.h \
    $$PWD/private/sshtunneloutsrv.h \
    $$PWD/private/sshserviceport.h \
    $$PWD/private/sshscpsend.h \
    $$PWD/private/sshsftp.h \
    $$PWD/sshworker.h \
    $$PWD/sshfs.h \
    $$PWD/sshaccess.h

SOURCES += \
    $$PWD/private/sshtunnelout.cpp \
    $$PWD/private/sshtunnelin.cpp \
    $$PWD/private/sshprocess.cpp \
    $$PWD/private/sshchannel.cpp \
    $$PWD/private/sshclient.cpp \
    $$PWD/private/sshtunneloutsrv.cpp \
    $$PWD/private/sshscpsend.cpp \
    $$PWD/private/sshsftp.cpp \
    $$PWD/private/sshworker.cpp

INCLUDEPATH += $$PWD/ $$PWD/private/
