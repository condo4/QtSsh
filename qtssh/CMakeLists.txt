cmake_minimum_required(VERSION 3.10)
project(QtSsh VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)
set(QT_VERSION ${Qt${QT_VERSION_MAJOR}_VERSION})

if(WITH_SSH_LIBRARIES)
    message("USE External SSH Headers ${WITH_SSH_HEADERS}")
    include_directories(${WITH_SSH_HEADERS})
    set(SSH2_LIBRARIES ${WITH_SSH_LIBRARIES})
else(WITH_SSH_LIBRARIES)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(SSH2 REQUIRED libssh2)
    link_directories(${SSH2_LIBRARY_DIRS})
endif(WITH_SSH_LIBRARIES)

set(SOURCES
	sshtunnelout.cpp
	sshtunnelin.cpp
	sshprocess.cpp
	sshchannel.cpp
	sshclient.cpp
	sshkey.cpp
	sshscpsend.cpp
	sshsftp.cpp
	sshtunnelout.cpp
	sshtunneloutconnection.cpp
	sshtunnelin.cpp
	sshtunnelinconnection.cpp
	sshsftpcommand.cpp
	sshsftpcommandmkdir.cpp
	sshsftpcommandunlink.cpp
	sshscpget.cpp
	sshsftpcommandfileinfo.cpp
	sshsftpcommandreaddir.cpp
	sshsftpcommandget.cpp
	sshsftpcommandsend.cpp
	sshtunneldataconnector.cpp
)

set(HEADERS
	sshtunnelout.h
	sshtunnelin.h
	sshprocess.h
	sshchannel.h
	sshclient.h
	sshkey.h
	sshscpsend.h
	sshsftp.h
	sshtunnelout.h
	sshtunneloutconnection.h
	sshtunnelin.h
	sshtunnelinconnection.h
	sshsftpcommand.h
	sshsftpcommandmkdir.h
	sshsftpcommandunlink.h
	sshscpget.h
	sshsftpcommandfileinfo.h
	sshsftpcommandreaddir.h
	sshsftpcommandget.h
	sshsftpcommandsend.h
	sshtunneldataconnector.h
)

if(BUILD_STATIC)
	add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
else(BUILD_STATIC)
	add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
endif(BUILD_STATIC)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SSH2_LIBRARIES} ${QT_LIBRARIES})

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
	$<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_include_directories(${PROJECT_NAME} PUBLIC ${SSH2_INCLUDE_DIRS})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_SSHCLIENT)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
)
install(EXPORT ${PROJECT_NAME}Targets
	DESTINATION lib/cmake
)
