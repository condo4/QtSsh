cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PV)
    set(PV "1.3.4")
endif(NOT DEFINED PV)

project(qtssh VERSION ${PV})

option(BUILD_STATIC  "Build static library"            OFF)
option(WITH_EXAMPLES "Build with examples"             OFF)

set(MESSAGE "${PROJECT_NAME}: ${PV}")

if(BUILD_STATIC)
    string(APPEND MESSAGE " static")
else(BUILD_STATIC)
    string(APPEND MESSAGE " shared")
endif(BUILD_STATIC)

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
