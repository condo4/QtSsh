# SshClient API Documentation

## Overview

`SshClient` is the main class for managing SSH connections in QtSsh. It provides the foundation for all SSH operations including authentication, connection management, and channel creation.

## Class Hierarchy

```
QObject
  └── SshClient
```

## Header

```cpp
#include "sshclient.h"
```

## Constructor

```cpp
SshClient(const QString &name = "noname", QObject *parent = nullptr)
```

Creates a new SSH client instance.

**Parameters:**
- `name`: Optional identifier for this client instance (useful for debugging)
- `parent`: Optional parent QObject

## Connection Management

### connectToHost

```cpp
int connectToHost(const QString &username,
                  const QString &hostname,
                  quint16 port = 22,
                  QByteArrayList methodes = QByteArrayList(),
                  int connTimeoutMsec = 60000)
```

Initiates a connection to an SSH server.

**Parameters:**
- `username`: Username for authentication
- `hostname`: Server hostname or IP address
- `port`: SSH port (default: 22)
- `methodes`: List of preferred authentication methods
- `connTimeoutMsec`: Connection timeout in milliseconds (default: 60000)

**Returns:** Status code (0 on success)

**Example:**
```cpp
SshClient *client = new SshClient("myConnection");
client->connectToHost("user", "example.com", 22);
```

### disconnectFromHost

```cpp
void disconnectFromHost()
```

Disconnects from the SSH server and closes all active channels.

### waitForState

```cpp
bool waitForState(SshClient::SshState state)
```

Blocks until the client reaches the specified state.

**Parameters:**
- `state`: Target SSH state to wait for

**Returns:** `true` if state was reached, `false` on timeout

**Example:**
```cpp
client->connectToHost("user", "example.com");
if (client->waitForState(SshClient::Ready)) {
    // Connection established
}
```

## Authentication

### setKeys

```cpp
void setKeys(const QString &publicKey, const QString &privateKey)
```

Sets SSH key pair for public key authentication.

**Parameters:**
- `publicKey`: Path to public key file
- `privateKey`: Path to private key file

**Example:**
```cpp
client->setKeys("/home/user/.ssh/id_rsa.pub", "/home/user/.ssh/id_rsa");
```

### setPassphrase

```cpp
void setPassphrase(const QString &pass)
```

Sets the passphrase for password authentication or encrypted private keys.

**Parameters:**
- `pass`: Password or passphrase

## Known Hosts Management

### setKownHostFile

```cpp
void setKownHostFile(const QString &file)
```

Sets the path to the known_hosts file for host verification.

**Parameters:**
- `file`: Path to known_hosts file

### addKnownHost

```cpp
bool addKnownHost(const QString &hostname, const SshKey &key)
```

Adds a host key to the known hosts list.

**Parameters:**
- `hostname`: Host to add
- `key`: Host's public key

**Returns:** `true` on success

### saveKnownHosts

```cpp
bool saveKnownHosts(const QString &file)
```

Saves the known hosts list to a file.

**Parameters:**
- `file`: Output file path

**Returns:** `true` on success

## Channel Management

### getChannel

```cpp
template<typename T>
T *getChannel(const QString &name)
```

Gets or creates a channel of the specified type.

**Template Parameters:**
- `T`: Channel type (SshProcess, SshSFtp, SshScpGet, SshScpSend, SshTunnelIn, SshTunnelOut)

**Parameters:**
- `name`: Unique name for the channel

**Returns:** Pointer to the channel

**Example:**
```cpp
// Get or create a process channel
SshProcess *proc = client->getChannel<SshProcess>("myProcess");

// Get or create an SFTP channel
SshSFtp *sftp = client->getChannel<SshSFtp>("myFtp");
```

## Properties

### getName

```cpp
QString getName() const
```

Returns the name of this SSH client instance.

### sshState

```cpp
SshState sshState() const
```

Returns the current SSH connection state.

**States:**
- `Unconnected`: Not connected
- `SocketConnection`: Establishing TCP connection
- `WaitingSocketConnection`: Waiting for TCP connection
- `Initialize`: Initializing SSH session
- `HandShake`: Performing SSH handshake
- `GetAuthenticationMethodes`: Retrieving authentication methods
- `Authentication`: Authenticating user
- `Ready`: Connected and ready
- `DisconnectingChannel`: Closing channels
- `DisconnectingSession`: Closing SSH session
- `FreeSession`: Freeing resources
- `Error`: Error state

### setName

```cpp
void setName(const QString &name)
```

Sets the name for this client instance.

### session

```cpp
LIBSSH2_SESSION *session()
```

Returns the underlying libssh2 session pointer (for advanced use).

## Configuration

### setProxy

```cpp
void setProxy(QNetworkProxy *proxy)
```

Sets a network proxy for the connection.

**Parameters:**
- `proxy`: Pointer to QNetworkProxy object

### setConnectTimeout

```cpp
void setConnectTimeout(int timeoutMsec)
```

Sets the connection timeout.

**Parameters:**
- `timeoutMsec`: Timeout in milliseconds

## Signals

### sshStateChanged

```cpp
void sshStateChanged(SshState sshState)
```

Emitted when the SSH connection state changes.

### sshReady

```cpp
void sshReady()
```

Emitted when the SSH connection is established and ready for use.

### sshDisconnected

```cpp
void sshDisconnected()
```

Emitted when disconnected from the SSH server.

### sshError

```cpp
void sshError()
```

Emitted when an SSH error occurs.

### sshDataReceived

```cpp
void sshDataReceived()
```

Emitted when data is received from the SSH server.

### channelsChanged

```cpp
void channelsChanged(int count)
```

Emitted when the number of active channels changes.

## Error Handling

### resetError

```cpp
void resetError()
```

Clears the current error state.

### Error Codes

The `sshErrorToString()` helper function converts libssh2 error codes to human-readable strings:

```cpp
const char* sshErrorToString(int err)
```

**Example:**
```cpp
connect(client, &SshClient::sshError, [=]() {
    qDebug() << "SSH Error occurred";
});
```

## Complete Example

```cpp
#include "sshclient.h"
#include "sshprocess.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SshClient *client = new SshClient("example");

    // Set authentication
    client->setPassphrase("mypassword");
    // or use keys:
    // client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");

    // Connect to signals
    QObject::connect(client, &SshClient::sshReady, [=]() {
        qDebug() << "Connected!";

        // Execute command
        SshProcess *proc = client->getChannel<SshProcess>("ls");
        proc->runCommand("ls -la");

        QObject::connect(proc, &SshProcess::finished, [=]() {
            qDebug() << proc->result();
            client->disconnectFromHost();
        });
    });

    QObject::connect(client, &SshClient::sshError, [=]() {
        qDebug() << "Connection error";
        app.quit();
    });

    QObject::connect(client, &SshClient::sshDisconnected, [=]() {
        qDebug() << "Disconnected";
        app.quit();
    });

    // Connect
    client->connectToHost("username", "hostname.com");

    return app.exec();
}
```

## Thread Safety

`SshClient` uses internal mutexes for channel creation but is primarily designed for single-threaded use with Qt's event loop.

## See Also

- [SshChannel](sshchannel.md) - Base channel class
- [SshProcess](sshprocess.md) - Remote command execution
- [SshSFtp](sshsftp.md) - SFTP file operations
- [Quick Start Guide](quickstart.md)
