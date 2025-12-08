# QtSsh Installation and Building Guide

## Overview

QtSsh is a Qt library wrapper for libssh2 that provides SSH functionality in Qt applications. This guide covers installation, building, and integration into your projects.

## Prerequisites

### Required Dependencies

- **Qt 5.x**: Qt Core and Qt Network modules
- **libssh2**: SSH2 protocol library
- **CMake** 3.13 or higher (for CMake builds)
- **qmake** (for qmake builds)
- C++11 compatible compiler

### Optional Dependencies

- **OpenSSL**: Required by libssh2 for encryption
- **zlib**: For compression support in libssh2

## Installing libssh2

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install libssh2-1-dev
```

### Fedora/RHEL/CentOS

```bash
sudo dnf install libssh2-devel
# or on older systems:
sudo yum install libssh2-devel
```

### macOS

Using Homebrew:
```bash
brew install libssh2
```

Using MacPorts:
```bash
sudo port install libssh2
```

### Windows

#### Using vcpkg

```bash
vcpkg install libssh2
```

#### Manual Installation

1. Download libssh2 from [https://www.libssh2.org/](https://www.libssh2.org/)
2. Build and install following the libssh2 documentation
3. Ensure the library and headers are in your compiler's search paths

## Building QtSsh

QtSsh can be built using either CMake or qmake.

### Method 1: CMake (Recommended)

#### Basic Build

```bash
# Clone or extract QtSsh
cd qtssh

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Install (optional)
sudo cmake --install .
```

#### Build Options

**Build static library:**
```bash
cmake -DBUILD_STATIC=ON ..
```

**Build with examples:**
```bash
cmake -DWITH_EXAMPLES=ON ..
```

**Specify custom libssh2 location:**
```bash
cmake -DWITH_SSH_LIBRARIES=/path/to/libssh2.so \
      -DWITH_SSH_HEADERS=/path/to/libssh2/include ..
```

**Complete example with all options:**
```bash
mkdir build && cd build
cmake -DBUILD_STATIC=OFF \
      -DWITH_EXAMPLES=ON \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release \
      ..
cmake --build . -j$(nproc)
sudo cmake --install .
```

### Method 2: qmake

#### As a Git Submodule

1. **Add QtSsh as submodule:**
```bash
cd your-project
git submodule add https://github.com/[repo]/qtssh.git
git submodule update --init --recursive
```

2. **In your .pro file:**
```qmake
include(qtssh/QtSsh.pri)

# Link with libssh2
unix {
    LIBS += -lssh2
}

win32 {
    LIBS += -lssh2
}
```

3. **Build your project:**
```bash
qmake
make
```

#### Standalone Build

```bash
cd qtssh
qmake QtSsh.pro
make
```

## Integration into Your Project

### CMake Integration

#### Option 1: Installed QtSsh

If QtSsh is installed system-wide:

```cmake
cmake_minimum_required(VERSION 3.13)
project(MyApp)

find_package(Qt5 REQUIRED COMPONENTS Core Network)
find_package(qtssh REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp Qt5::Core Qt5::Network qtssh)
```

#### Option 2: Embedded QtSsh

Add QtSsh as a subdirectory:

```cmake
cmake_minimum_required(VERSION 3.13)
project(MyApp)

find_package(Qt5 REQUIRED COMPONENTS Core Network)

# Add QtSsh
add_subdirectory(qtssh)

add_executable(myapp main.cpp)
target_link_libraries(myapp Qt5::Core Qt5::Network qtssh)
```

#### Option 3: External Project

```cmake
include(qtssh/qtssh.cmake)

add_executable(myapp main.cpp)
target_link_libraries(myapp qtssh)
```

### qmake Integration

In your `.pro` file:

```qmake
QT += core network

# Include QtSsh
include(path/to/qtssh/QtSsh.pri)

# Link with libssh2
unix {
    LIBS += -lssh2
}

win32 {
    # Adjust path as needed
    LIBS += -lssh2
    # Or for static:
    # LIBS += -Lc:/path/to/libssh2/lib -lssh2
}

SOURCES += main.cpp
```

### Manual Integration

1. **Add QtSsh source files to your project:**
   - Copy all files from `qtssh/` directory
   - Add them to your project's build system

2. **Include headers:**
```cpp
#include "sshclient.h"
#include "sshprocess.h"
#include "sshsftp.h"
// etc.
```

3. **Link with libssh2:**
   - Add `-lssh2` to linker flags
   - Ensure libssh2 headers are in include path

## Project Structure

```
qtssh/
├── CMakeLists.txt          # CMake configuration
├── QtSsh.pri              # qmake configuration
├── qtssh.cmake            # CMake helper
├── LICENSE                # BSD 3-Clause License
├── README.md              # Basic readme
├── doc/                   # Documentation (this folder)
├── qtssh/                 # Library source code
│   ├── sshclient.h
│   ├── sshclient.cpp
│   ├── sshprocess.h
│   ├── sshprocess.cpp
│   ├── sshsftp.h
│   ├── sshsftp.cpp
│   ├── sshscpget.h
│   ├── sshscpget.cpp
│   ├── sshscpsend.h
│   ├── sshscpsend.cpp
│   ├── sshtunnelin.h
│   ├── sshtunnelin.cpp
│   ├── sshtunnelout.h
│   ├── sshtunnelout.cpp
│   └── ...
└── test/                  # Test applications
    ├── SshConsole/        # Console test app
    ├── SshGui/            # GUI test app
    └── TestSsh/           # Unit tests
```

## Verifying Installation

### Simple Test Program

Create a file `test.cpp`:

```cpp
#include <QCoreApplication>
#include <QDebug>
#include "sshclient.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SshClient client("test");
    qDebug() << "QtSsh is working!";
    qDebug() << "Client name:" << client.getName();

    return 0;
}
```

#### Build with CMake:

`CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.13)
project(test)

find_package(Qt5 REQUIRED COMPONENTS Core Network)
find_package(qtssh REQUIRED)

add_executable(test test.cpp)
target_link_libraries(test Qt5::Core Qt5::Network qtssh)
```

Build:
```bash
mkdir build && cd build
cmake ..
cmake --build .
./test
```

#### Build with qmake:

`test.pro`:
```qmake
QT += core network
CONFIG += console
CONFIG -= app_bundle

include(../qtssh/QtSsh.pri)

SOURCES += test.cpp

unix {
    LIBS += -lssh2
}
```

Build:
```bash
qmake
make
./test
```

## Platform-Specific Notes

### Linux

- Standard build process works well
- libssh2 usually available in package managers
- No special configuration needed

### macOS

- Use Homebrew or MacPorts for dependencies
- May need to specify library paths:
```bash
cmake -DCMAKE_PREFIX_PATH=/usr/local/opt/libssh2 ..
```

### Windows

- Ensure libssh2 DLL is in PATH or application directory
- For static builds, define `LIBSSH2_STATIC` if needed
- Visual Studio, MinGW, and MSVC are supported

**Visual Studio example:**
```bash
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

**MinGW example:**
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

## Troubleshooting

### libssh2 Not Found

**Error:** `Could not find libssh2`

**Solution:**
```bash
# Specify libssh2 location
cmake -DWITH_SSH_LIBRARIES=/path/to/libssh2 \
      -DWITH_SSH_HEADERS=/path/to/libssh2/include ..
```

### Qt Not Found

**Error:** `Could not find Qt5`

**Solution:**
```bash
# Specify Qt location
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/gcc_64 ..
```

### Linker Errors on Windows

**Error:** Unresolved symbols from libssh2

**Solution:**
- Ensure libssh2.lib is linked
- Check if static/dynamic linking matches
- Add OpenSSL and zlib if libssh2 requires them

### Runtime Errors

**Error:** `Cannot load library libssh2`

**Solution:**
- Ensure libssh2 shared library is in system path
- On Windows: Copy libssh2.dll to application directory
- On Linux: Check `LD_LIBRARY_PATH`
- On macOS: Check `DYLD_LIBRARY_PATH`

## Building Documentation

Documentation is written in Markdown and located in the `doc/` folder.

To convert to HTML (requires a Markdown processor):
```bash
# Using pandoc
for file in doc/*.md; do
    pandoc "$file" -o "${file%.md}.html"
done
```

## Running Tests

```bash
cd test/TestSsh
qmake
make
./TestSsh
```

Or with CMake:
```bash
mkdir build && cd build
cmake -DWITH_EXAMPLES=ON ..
cmake --build .
./bin/TestSsh
```

## Next Steps

- See [Quick Start Guide](quickstart.md) for your first QtSsh application
- Read [API Documentation](sshclient.md) for detailed class references
- Check out example applications in the `test/` directory

## License

QtSsh is licensed under the BSD 3-Clause License. See [LICENSE](../LICENSE) file for details.

## Support

- Report issues on GitHub issue tracker
- Check example applications for usage patterns
- Consult libssh2 documentation for underlying SSH functionality
