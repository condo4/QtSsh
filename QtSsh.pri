# QtSsh
HEADERS += \
    $$PWD/qtssh/sshtunnelout.h \
    $$PWD/qtssh/sshtunnelin.h \
    $$PWD/qtssh/sshprocess.h \
    $$PWD/qtssh/sshchannel.h \
    $$PWD/qtssh/sshclient.h \
    $$PWD/qtssh/sshtunneloutsrv.h \
    $$PWD/qtssh/sshscpsend.h \
    $$PWD/qtssh/sshsftp.h \
    $$PWD/qtssh/sshworker.h \
    $$PWD/qtssh/sshinterface.h \
    $$PWD/qtssh/sshfsinterface.h \
    $$PWD/qtssh/sshfilesystemmodel.h \
    $$PWD/qtssh/sshfilesystemnode.h

SOURCES += \
    $$PWD/qtssh/sshtunnelout.cpp \
    $$PWD/qtssh/sshtunnelin.cpp \
    $$PWD/qtssh/sshprocess.cpp \
    $$PWD/qtssh/sshchannel.cpp \
    $$PWD/qtssh/sshclient.cpp \
    $$PWD/qtssh/sshtunneloutsrv.cpp \
    $$PWD/qtssh/sshscpsend.cpp \
    $$PWD/qtssh/sshsftp.cpp \
    $$PWD/qtssh/sshworker.cpp \
    $$PWD/qtssh/sshfilesystemmodel.cpp \
    $$PWD/qtssh/sshfilesystemnode.cpp

INCLUDEPATH += $$PWD/qtssh
