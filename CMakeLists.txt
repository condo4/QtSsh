cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PV)
    set(PV "1.3.4")
endif(NOT DEFINED PV)

project(qtssh VERSION ${PV})

option(BUILD_STATIC  "Build static library"            OFF)
option(WITH_EXAMPLES "Build with examples"             OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

set(CMakeModuleName "QtSsh")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${CMakeModuleName}ConfigVersion.cmake")
set(project_config "${generated_dir}/${CMakeModuleName}Config.cmake")
set(targets_export_name "${CMakeModuleName}Targets")
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(MESSAGE "${PROJECT_NAME}: ${PV}")

if(BUILD_STATIC)
    string(APPEND MESSAGE " static")
else(BUILD_STATIC)
    string(APPEND MESSAGE " shared")
endif(BUILD_STATIC)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)
set(QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)


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
	qtssh/sshtunnelout.cpp
	qtssh/sshtunnelin.cpp
	qtssh/sshprocess.cpp
	qtssh/sshchannel.cpp
	qtssh/sshclient.cpp
	qtssh/sshkey.cpp
	qtssh/sshscpsend.cpp
	qtssh/sshsftp.cpp
	qtssh/sshtunnelout.cpp
	qtssh/sshtunneloutconnection.cpp
	qtssh/sshtunnelin.cpp
	qtssh/sshtunnelinconnection.cpp
	qtssh/sshsftpcommand.cpp
	qtssh/sshsftpcommandmkdir.cpp
	qtssh/sshsftpcommandunlink.cpp
	qtssh/sshscpget.cpp
	qtssh/sshsftpcommandfileinfo.cpp
	qtssh/sshsftpcommandreaddir.cpp
	qtssh/sshsftpcommandget.cpp
	qtssh/sshsftpcommandsend.cpp
	qtssh/sshtunneldataconnector.cpp
)


set(HEADERS
	qtssh/sshtunnelout.h
	qtssh/sshtunnelin.h
	qtssh/sshprocess.h
	qtssh/sshchannel.h
	qtssh/sshclient.h
	qtssh/sshkey.h
	qtssh/sshscpsend.h
	qtssh/sshsftp.h
	qtssh/sshtunnelout.h
	qtssh/sshtunneloutconnection.h
	qtssh/sshtunnelin.h
	qtssh/sshtunnelinconnection.h
	qtssh/sshsftpcommand.h
	qtssh/sshsftpcommandmkdir.h
	qtssh/sshsftpcommandunlink.h
	qtssh/sshscpget.h
	qtssh/sshsftpcommandfileinfo.h
	qtssh/sshsftpcommandreaddir.h
	qtssh/sshsftpcommandget.h
	qtssh/sshsftpcommandsend.h
	qtssh/sshtunneldataconnector.h
)


if(BUILD_STATIC)
	add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
else(BUILD_STATIC)
	add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
	set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)
endif(BUILD_STATIC)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SSH2_LIBRARIES} ${QT_LIBRARIES})

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
	$<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${SSH2_INCLUDE_DIRS}>)
target_include_directories(${PROJECT_NAME} PRIVATE qtssh/)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_SSHCLIENT)

install(TARGETS ${PROJECT_NAME} EXPORT "${targets_export_name}"
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
)



if (WITH_EXAMPLES)
	add_subdirectory(examples)
endif()

# create Config.cmake
# CMake Module
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/generated/QtSshConfigVersion.cmake" COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

install(
    EXPORT "${targets_export_name}"
    NAMESPACE qtssh::
    DESTINATION "${config_install_dir}"
)

