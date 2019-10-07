cmake_minimum_required(VERSION 3.0)
project(qtssh VERSION 0.1)

option(BUILD_STATIC  "Build static library"            OFF)

if(BUILD_STATIC)
    message(STATUS "Build QtSsh static")
else(BUILD_STATIC)
    message(STATUS "Build QtSsh shared")
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


add_subdirectory(qtssh)

if (WithExamples)
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
