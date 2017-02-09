
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(UseQt5 "UseQt5" ON)
if (UseQt5)
	find_package(Qt5 REQUIRED COMPONENTS Core Network)
	set(QT_LIBRARIES Qt5::Core Qt5::Network)
	set(QT_VERSION ${Qt5_VERSION})
else()
	find_package(Qt4 REQUIRED COMPONENTS QtCore QtNetwork)
	set(QT_VERSION ${Qt4_VERSION})
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(SSH2 REQUIRED libssh2)
link_directories(${SSH2_LIBRARY_DIRS})

set(SOURCES
	sshtunnelout.cpp
	sshtunnelin.cpp
	sshprocess.cpp
	sshchannel.cpp
	sshclient.cpp
	sshtunneloutsrv.cpp
	sshscpsend.cpp
	sshsftp.cpp
	sshworker.cpp
	sshfilesystemmodel.cpp
	sshfilesystemnode.cpp
)

set(HEADERS
	sshclient.h
	sshworker.h
	sshinterface.h
	sshfsinterface.h
	sshfilesystemmodel.h
	sshserviceport.h
)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
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
