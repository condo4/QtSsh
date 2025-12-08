# QtSsh

A Qt library wrapper for libssh2 providing comprehensive SSH functionality including remote command execution, file transfer (SFTP/SCP), and SSH tunneling.

## Features

- **Remote Command Execution**: Execute commands on remote servers via SSH
- **SFTP Support**: Full SFTP implementation for file operations
  - Upload and download files
  - Directory operations (create, list, remove)
  - File attribute queries
- **SCP File Transfer**: Secure file copy with progress monitoring
- **SSH Tunneling**: Both local and remote port forwarding
  - Local forwarding (access remote services)
  - Remote forwarding (expose local services)
- **Authentication**: Support for both password and public key authentication
- **Qt Integration**: Native Qt signals/slots and event-driven architecture
- **Channel Management**: Reusable channels for efficient operations

## Quick Example

```cpp
#include <QCoreApplication>
#include "sshclient.h"
#include "sshprocess.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SshClient *client = new SshClient("example");
    client->setPassphrase("password");

    QObject::connect(client, &SshClient::sshReady, [=]() {
        SshProcess *proc = client->getChannel<SshProcess>("cmd");
        proc->runCommand("ls -la");

        QObject::connect(proc, &SshProcess::finished, [=]() {
            qDebug() << proc->result();
            client->disconnectFromHost();
        });
    });

    QObject::connect(client, &SshClient::sshDisconnected, [&]() {
        app.quit();
    });

    client->connectToHost("user", "server.com");
    return app.exec();
}
```

## Documentation

### Getting Started

- **[Installation Guide](doc/installation.md)** - Build and integrate QtSsh into your project
- **[Quick Start Guide](doc/quickstart.md)** - Your first QtSsh application with examples
- **[Error Handling](doc/errors.md)** - Comprehensive error handling guide

### API Reference

- **[SshClient](doc/sshclient.md)** - Main SSH client class for connection management
- **[SshChannel](doc/sshchannel.md)** - Base class for all channel types
- **[SshProcess](doc/sshprocess.md)** - Remote command execution
- **[SshSFtp](doc/sshsftp.md)** - SFTP file transfer and operations
- **[SshScpGet/SshScpSend](doc/sshscp.md)** - SCP file transfer with progress
- **[SSH Tunneling](doc/tunneling.md)** - Local and remote port forwarding

## Requirements

- Qt 5.x (Core and Network modules)
- libssh2
- C++11 compatible compiler
- CMake 3.13+ or qmake

## Installation

### Quick Install

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libssh2-1-dev qt5-default

# Build QtSsh
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

### Integration into Your Project

#### CMake

```cmake
find_package(qtssh REQUIRED)
target_link_libraries(your_app qtssh)
```

#### qmake

```qmake
include(path/to/QtSsh.pri)
LIBS += -lssh2
```

See the [Installation Guide](doc/installation.md) for detailed instructions.

## Usage Examples

### Execute Remote Command

```cpp
SshProcess *proc = client->getChannel<SshProcess>("command");
proc->runCommand("uptime");

connect(proc, &SshProcess::finished, [=]() {
    qDebug() << "Output:" << proc->result();
});
```

### Upload File via SFTP

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("upload");
QString error = sftp->send("/local/file.txt", "/remote/file.txt");

if (error.isEmpty()) {
    qDebug() << "Upload successful";
}
```

### Download File with Progress

```cpp
SshScpGet *scp = client->getChannel<SshScpGet>("download");

connect(scp, &SshScpGet::progress, [](qint64 received, qint64 total) {
    qDebug() << "Progress:" << (received * 100 / total) << "%";
});

connect(scp, &SshScpGet::finished, []() {
    qDebug() << "Download complete";
});

scp->get("/remote/file.zip", "/local/file.zip");
```

### Create SSH Tunnel

```cpp
SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("db-tunnel");
tunnel->listen(3306, "mysql.internal.network");

qDebug() << "MySQL tunnel ready at localhost:3306";
```

## Project Structure

```
qtssh/
├── CMakeLists.txt        # CMake build configuration
├── QtSsh.pri            # qmake configuration
├── README.md            # This file
├── LICENSE              # BSD 3-Clause License
├── doc/                 # Documentation
│   ├── installation.md  # Build and installation guide
│   ├── quickstart.md    # Getting started tutorial
│   ├── errors.md        # Error handling guide
│   ├── sshclient.md     # SshClient API reference
│   ├── sshchannel.md    # SshChannel API reference
│   ├── sshprocess.md    # SshProcess API reference
│   ├── sshsftp.md       # SshSFtp API reference
│   ├── sshscp.md        # SshScp API reference
│   └── tunneling.md     # Tunneling guide
├── qtssh/               # Library source code
└── test/                # Example applications
    ├── SshConsole/      # Console SSH client
    ├── SshGui/          # GUI SSH client
    └── TestSsh/         # Unit tests
```

## Key Classes

| Class | Purpose |
|-------|---------|
| `SshClient` | Main SSH connection management |
| `SshProcess` | Execute remote commands |
| `SshSFtp` | SFTP file operations |
| `SshScpGet` | Download files via SCP |
| `SshScpSend` | Upload files via SCP |
| `SshTunnelOut` | Local port forwarding |
| `SshTunnelIn` | Remote port forwarding |
| `SshChannel` | Base class for all channels |
| `SshKey` | SSH key management |

## Authentication Methods

### Password Authentication

```cpp
client->setPassphrase("your-password");
client->connectToHost("username", "hostname");
```

### Public Key Authentication

```cpp
client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");
client->connectToHost("username", "hostname");
```

## License

QtSsh is licensed under the **BSD 3-Clause License**.

```
Copyright (c) 2018, Proriol Fabien
All rights reserved.
```

See [LICENSE](LICENSE) file for full license text.

## Contributing

This project is designed to be integrated as a git submodule in larger projects.

To use QtSsh in your project:

```bash
git submodule add <repository-url> qtssh
git submodule update --init --recursive
```

Then include in your build:
- **qmake**: `include(qtssh/QtSsh.pri)`
- **CMake**: `add_subdirectory(qtssh)` or use `qtssh.cmake`

## Version

Current version: **1.3.4**

## Dependencies

- **Qt 5.x**: Core and Network modules
- **libssh2**: SSH2 protocol implementation
  - Typically requires OpenSSL for encryption
  - Optional zlib support for compression

## Platform Support

- Linux (tested and recommended)
- macOS
- Windows (MinGW, MSVC)

## Example Applications

The `test/` directory contains example applications:

- **SshConsole**: Command-line SSH client
- **SshGui**: GUI application demonstrating all features
- **TestSsh**: Unit tests and examples

## Common Use Cases

- **Remote Server Management**: Execute commands on remote servers
- **Automated Deployments**: Upload files and run deployment scripts
- **Database Tunneling**: Secure access to remote databases
- **File Synchronization**: Backup and sync files between systems
- **Network Access**: Access internal network services through SSH gateway
- **Development Tools**: Remote debugging, log access, server monitoring

## Support and Resources

- **Documentation**: See `doc/` directory for comprehensive guides
- **Examples**: Check `test/` directory for working examples
- **libssh2 Documentation**: [https://www.libssh2.org/](https://www.libssh2.org/)
- **Qt Documentation**: [https://doc.qt.io/](https://doc.qt.io/)

## Building from Source

### Using CMake

```bash
mkdir build && cd build
cmake -DBUILD_STATIC=OFF -DWITH_EXAMPLES=ON ..
cmake --build .
sudo cmake --install .
```

### Using qmake

```bash
qmake QtSsh.pro
make
```

See [Installation Guide](doc/installation.md) for detailed build instructions.

## Getting Help

1. Check the [Quick Start Guide](doc/quickstart.md)
2. Read the [Error Handling Guide](doc/errors.md)
3. Review example applications in `test/` directory
4. Consult API reference documentation in `doc/`
5. Check libssh2 documentation for underlying SSH functionality

## Features Comparison

| Feature | QtSsh | Native libssh2 |
|---------|-------|----------------|
| Qt Integration | ✓ | ✗ |
| Signals/Slots | ✓ | ✗ |
| Channel Management | ✓ | Manual |
| Progress Reporting | ✓ | Manual |
| Error Handling | Comprehensive | Error codes |
| SFTP Operations | High-level API | Low-level API |
| Tunneling | Managed | Manual |

## Security Notes

- Always use key-based authentication in production
- Set appropriate file permissions (600 for private keys)
- Validate host keys using known_hosts file
- Use encrypted connections only (SSH protocol ensures this)
- Keep libssh2 updated for security patches
- Don't hardcode credentials in source code
- Use secure key storage mechanisms

## Performance Tips

- Reuse channels for multiple operations
- Use SCP for simple file transfers (faster than SFTP)
- Use SFTP for complex file operations
- Keep persistent connections for multiple operations
- Close unused channels to free resources
- Use appropriate buffer sizes for large transfers

## Roadmap

Current stable version: 1.3.4

For feature requests and issues, please use the project's issue tracker.

---

**Quick Links:**
- [Installation](doc/installation.md) | [Quick Start](doc/quickstart.md) | [API Reference](doc/sshclient.md) | [Examples](test/) | [License](LICENSE)
