import qbs


Product {
    type: "staticlibrary"
    name:"QtSsh"
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.network" }
    Depends { name: "libssh2" }

    files: [
        "qtssh/sshtunnelout.h",
        "qtssh/sshtunnelin.h",
        "qtssh/sshprocess.h",
        "qtssh/sshchannel.h",
        "qtssh/sshclient.h",
        "qtssh/sshtunneloutsrv.h",
        "qtssh/sshscpsend.h",
        "qtssh/sshsftp.h",
        "qtssh/sshfilesystemmodel.h",
        "qtssh/sshfilesystemnode.h",
        "qtssh/async_libssh2.h",
        "qtssh/sshkey.h",
        "qtssh/sshtunnelout.cpp",
        "qtssh/sshtunnelin.cpp",
        "qtssh/sshprocess.cpp",
        "qtssh/sshchannel.cpp",
        "qtssh/sshclient.cpp",
        "qtssh/sshtunneloutsrv.cpp",
        "qtssh/sshscpsend.cpp",
        "qtssh/sshsftp.cpp",
        "qtssh/sshfilesystemmodel.cpp",
        "qtssh/sshfilesystemnode.cpp",
        "qtssh/sshkey.cpp",
        "qtssh/sshscpget.cpp",
        "qtssh/sshscpget.h"
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            product.sourceDirectory + "/qtssh"
        ]
    }
}

