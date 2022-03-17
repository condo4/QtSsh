cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PV)
    set(PV "1.3.4")
endif(NOT DEFINED PV)

project(qtssh VERSION ${PV})

option(BUILD_STATIC  "Build static library"            OFF)


set(MESSAGE "${PROJECT_NAME}: ${PV}")

if(BUILD_STATIC)
    string(APPEND MESSAGE " static")
else(BUILD_STATIC)
    string(APPEND MESSAGE " shared")
endif(BUILD_STATIC)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network)
set(QT_LIBRARIES Qt5::Core Qt5::Network)
set(QT_VERSION ${Qt5_VERSION})

if(WITH_SSH_LIBRARIES)
	if(WITH_SSH_HEADERS)
		string(APPEND MESSAGE " libssh2 External Headers ${WITH_SSH_HEADERS}")
	endif(WITH_SSH_HEADERS)
	string(APPEND MESSAGE " libssh2 External Library ${WITH_SSH_LIBRARIES}")
endif(WITH_SSH_LIBRARIES)

add_subdirectory(qtssh)

if (WITH_EXAMPLES)
	add_subdirectory(examples)
endif()

# create Config.cmake
configure_file(config.cmake.in "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

# create ConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion)

# install the *Config.cmake and *ConfigVersion.cmake
install(FILES
	"${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	"${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION lib/cmake
)

message(STATUS ${MESSAGE})
