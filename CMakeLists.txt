cmake_minimum_required(VERSION 3.0)
project(qtssh VERSION 0.1)

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

add_subdirectory(qtssh)
add_subdirectory(examples)

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
