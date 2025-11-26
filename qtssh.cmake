set(QTSSH_FILES
    ${CMAKE_CURRENT_LIST_DIR}/qtssh.cmake
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelout.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshprocess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshchannel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshclient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshkey.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshscpsend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelout.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunneloutconnection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandreaddir.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelinconnection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandsend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandfileinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandunlink.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshscpget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandmkdir.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunneldataconnector.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelout.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelin.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshprocess.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshchannel.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshclient.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshkey.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshscpsend.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftp.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelout.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunneloutconnection.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelin.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandreaddir.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunnelinconnection.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandfileinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandsend.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandget.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandunlink.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommand.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshscpget.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshsftpcommandmkdir.h
    ${CMAKE_CURRENT_LIST_DIR}/qtssh/sshtunneldataconnector.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/qtssh)

if(NOT DEFINED LIBSSH2_LIBRARIES)
    message(STATUS "Search for SSH2")
    find_package(PkgConfig REQUIRED)
    pkg_search_module(SSH2 REQUIRED libssh2)
endif()
