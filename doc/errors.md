# Error Handling Guide

## Overview

Proper error handling is essential for robust SSH applications. QtSsh provides multiple mechanisms for detecting and handling errors at both the connection and channel levels.

## Error Detection Methods

### 1. Signals

The primary method for asynchronous error detection.

#### SshClient Signals

```cpp
connect(client, &SshClient::sshError, [=]() {
    qDebug() << "SSH connection error occurred";
});

connect(client, &SshClient::sshDisconnected, [=]() {
    qDebug() << "Disconnected from server";
});
```

#### SshProcess Signals

```cpp
connect(proc, &SshProcess::failed, [=]() {
    qDebug() << "Command execution failed";
    qDebug() << "Errors:" << proc->errMsg();
});
```

#### SshScpGet/SshScpSend Signals

```cpp
connect(scp, &SshScpSend::failed, [=]() {
    qDebug() << "File transfer failed";
});
```

### 2. State Checking

Monitor connection and channel states.

#### Client States

```cpp
SshClient::SshState state = client->sshState();

switch(state) {
    case SshClient::Error:
        qDebug() << "Client in error state";
        break;
    case SshClient::Ready:
        qDebug() << "Client ready";
        break;
    case SshClient::Unconnected:
        qDebug() << "Not connected";
        break;
    // ... other states
}
```

#### Channel States

```cpp
SshChannel::ChannelState state = channel->channelState();

if (state == SshChannel::Error) {
    qDebug() << "Channel error";
}
```

### 3. Return Value Checking

Some methods return values indicating success or failure.

```cpp
// SFTP operations
SshSFtp *sftp = client->getChannel<SshSFtp>("upload");
QString error = sftp->send(local, remote);

if (!error.isEmpty()) {
    qDebug() << "Upload failed:" << error;
}

// Check error flag
if (sftp->isError()) {
    qDebug() << "Errors:" << sftp->errMsg();
}
```

### 4. Wait Operations

Timeout-based error detection.

```cpp
bool connected = client->waitForState(SshClient::Ready);
if (!connected) {
    qDebug() << "Connection timeout";
}
```

## Common Errors and Solutions

### Connection Errors

#### Error: Cannot Connect to Host

**Symptoms:**
- `sshError` signal emitted
- State becomes `SshClient::Error`

**Possible Causes:**
1. Wrong hostname or IP address
2. Port blocked by firewall
3. SSH service not running
4. Network connectivity issues

**Solutions:**
```cpp
client->setConnectTimeout(30000);  // Increase timeout

// Test connectivity first
QTcpSocket testSocket;
testSocket.connectToHost("hostname", 22);
if (!testSocket.waitForConnected(5000)) {
    qDebug() << "Cannot reach host";
}
```

#### Error: Authentication Failed

**Symptoms:**
- `sshError` signal after connection attempt
- State changes to `Error` during `Authentication` phase

**Possible Causes:**
1. Wrong username or password
2. Invalid SSH key
3. Key permissions too open (should be 600)
4. Server doesn't allow password authentication
5. Server requires different authentication method

**Solutions:**
```cpp
// Try password authentication
client->setPassphrase("correct-password");

// Try key authentication
client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");

// Check key permissions on Linux/macOS:
// chmod 600 ~/.ssh/id_rsa
// chmod 644 ~/.ssh/id_rsa.pub
```

#### Error: Host Key Verification Failed

**Symptoms:**
- Connection fails during handshake
- libssh2 reports host key mismatch

**Solutions:**
```cpp
// Set known hosts file
client->setKownHostFile("/home/user/.ssh/known_hosts");

// Or manually add host
SshKey hostKey = /* get from first connection */;
client->addKnownHost("hostname", hostKey);
client->saveKnownHosts("/home/user/.ssh/known_hosts");
```

### Channel Errors

#### Error: Channel Creation Fails

**Symptoms:**
- Channel remains in `Openning` state
- Operations don't execute

**Possible Causes:**
1. Client not in `Ready` state
2. Too many channels open
3. Server resource limits

**Solutions:**
```cpp
// Ensure client is ready
if (client->sshState() != SshClient::Ready) {
    qDebug() << "Wait for ready state first";
    return;
}

// Close unused channels
channel->close();
```

#### Error: Command Execution Fails

**Symptoms:**
- `failed` signal emitted
- `isError()` returns true

**Possible Causes:**
1. Command not found on remote system
2. Permission denied
3. Command syntax error
4. Connection lost during execution

**Solutions:**
```cpp
SshProcess *proc = client->getChannel<SshProcess>("test");

connect(proc, &SshProcess::failed, [=]() {
    qDebug() << "Command failed";
    QStringList errors = proc->errMsg();
    for (const QString &err : errors) {
        qDebug() << "  " << err;
    }
});

// Use full paths for commands
proc->runCommand("/usr/bin/ls -la");

// Check command exists first
proc->runCommand("which ls");
```

### File Transfer Errors

#### Error: SFTP Upload Fails

**Symptoms:**
- `send()` returns non-empty error string
- `isError()` returns true

**Possible Causes:**
1. Remote path doesn't exist
2. No write permissions
3. Disk full
4. Local file doesn't exist or can't be read

**Solutions:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("upload");

// Check local file exists
if (!QFile::exists(localPath)) {
    qDebug() << "Local file not found";
    return;
}

// Create remote directory first
QFileInfo remoteInfo(remotePath);
sftp->mkpath(remoteInfo.path());

// Upload
QString error = sftp->send(localPath, remotePath);
if (!error.isEmpty()) {
    qDebug() << "Upload error:" << error;
    if (sftp->isError()) {
        for (const QString &err : sftp->errMsg()) {
            qDebug() << "  " << err;
        }
    }
}
```

#### Error: SFTP Download Fails

**Symptoms:**
- `get()` returns false
- File not created locally

**Possible Causes:**
1. Remote file doesn't exist
2. No read permissions on remote
3. No write permissions locally
4. Insufficient local disk space

**Solutions:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("download");

// Check remote file exists
if (!sftp->isFile(remotePath)) {
    qDebug() << "Remote file doesn't exist";
    return;
}

// Check remote file size
quint64 size = sftp->filesize(remotePath);
qDebug() << "File size:" << size << "bytes";

// Ensure local directory exists
QFileInfo localInfo(localPath);
QDir().mkpath(localInfo.path());

// Download
bool success = sftp->get(remotePath, localPath, true);
if (!success) {
    qDebug() << "Download failed:" << sftp->errMsg();
}
```

#### Error: SCP Transfer Fails

**Symptoms:**
- `failed` signal emitted
- Transfer stops mid-way

**Solutions:**
```cpp
SshScpSend *scp = client->getChannel<SshScpSend>("upload");

connect(scp, &SshScpSend::failed, [=]() {
    qDebug() << "SCP upload failed";
    // Implement retry logic
});

connect(scp, &SshScpSend::progress, [=](qint64 sent, qint64 total) {
    // Monitor for stalls
    static qint64 lastSent = 0;
    if (sent == lastSent) {
        qDebug() << "Transfer stalled?";
    }
    lastSent = sent;
});

scp->send(localPath, remotePath);
```

### Tunneling Errors

#### Error: Tunnel Won't Start

**Symptoms:**
- `listen()` doesn't establish tunnel
- No `connectionChanged` signals

**Possible Causes:**
1. Port already in use
2. Permission denied (ports < 1024)
3. Server doesn't allow port forwarding
4. Firewall blocking

**Solutions:**
```cpp
SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("forward");

// Use auto-assigned port
tunnel->listen(0, "remote-host");  // Port 0 = auto
quint16 actualPort = tunnel->localPort();
qDebug() << "Tunnel on port:" << actualPort;

// Check port availability first
QTcpServer testServer;
if (!testServer.listen(QHostAddress::LocalHost, 8080)) {
    qDebug() << "Port 8080 already in use";
}
testServer.close();
```

## Error Handling Patterns

### Pattern 1: Comprehensive Error Handler

```cpp
class SshErrorHandler : public QObject {
    Q_OBJECT
public:
    SshErrorHandler(SshClient *client) : m_client(client) {
        connect(client, &SshClient::sshStateChanged,
                this, &SshErrorHandler::onStateChanged);
        connect(client, &SshClient::sshError,
                this, &SshErrorHandler::onError);
    }

private slots:
    void onStateChanged(SshClient::SshState state) {
        qDebug() << "State:" << state;

        if (state == SshClient::Error) {
            qDebug() << "Client entered error state";
            emit errorOccurred("Client error");
        }
    }

    void onError() {
        qDebug() << "SSH Error signal received";

        SshClient::SshState state = m_client->sshState();
        QString errorMsg;

        switch(state) {
            case SshClient::Authentication:
                errorMsg = "Authentication failed";
                break;
            case SshClient::HandShake:
                errorMsg = "Handshake failed";
                break;
            default:
                errorMsg = "Unknown error";
        }

        emit errorOccurred(errorMsg);
    }

signals:
    void errorOccurred(QString message);

private:
    SshClient *m_client;
};
```

### Pattern 2: Retry with Exponential Backoff

```cpp
class RetryConnector : public QObject {
    Q_OBJECT
public:
    RetryConnector(SshClient *client, int maxRetries = 3)
        : m_client(client), m_maxRetries(maxRetries), m_retries(0), m_backoff(1000) {

        connect(m_client, &SshClient::sshReady, this, &RetryConnector::onConnected);
        connect(m_client, &SshClient::sshError, this, &RetryConnector::onError);
    }

    void connect(const QString &user, const QString &host) {
        m_user = user;
        m_host = host;
        tryConnect();
    }

private slots:
    void onConnected() {
        qDebug() << "Connected successfully";
        m_retries = 0;
        emit connected();
    }

    void onError() {
        m_retries++;

        if (m_retries >= m_maxRetries) {
            qDebug() << "Max retries reached, giving up";
            emit failed();
            return;
        }

        int delay = m_backoff * (1 << (m_retries - 1));  // Exponential backoff
        qDebug() << "Retry" << m_retries << "in" << delay << "ms";

        QTimer::singleShot(delay, this, &RetryConnector::tryConnect);
    }

    void tryConnect() {
        qDebug() << "Attempting connection...";
        m_client->connectToHost(m_user, m_host);
    }

signals:
    void connected();
    void failed();

private:
    SshClient *m_client;
    int m_maxRetries;
    int m_retries;
    int m_backoff;
    QString m_user;
    QString m_host;
};
```

### Pattern 3: Graceful Degradation

```cpp
void robustFileTransfer(SshClient *client, const QString &local, const QString &remote) {
    // Try SFTP first (more reliable)
    SshSFtp *sftp = client->getChannel<SshSFtp>("transfer");
    QString error = sftp->send(local, remote);

    if (error.isEmpty()) {
        qDebug() << "SFTP transfer successful";
        return;
    }

    // Fall back to SCP
    qDebug() << "SFTP failed, trying SCP...";
    SshScpSend *scp = client->getChannel<SshScpSend>("transfer-scp");

    connect(scp, &SshScpSend::finished, []() {
        qDebug() << "SCP transfer successful";
    });

    connect(scp, &SshScpSend::failed, []() {
        qDebug() << "Both SFTP and SCP failed";
    });

    scp->send(local, remote);
}
```

### Pattern 4: Error Logging

```cpp
class SshLogger : public QObject {
    Q_OBJECT
public:
    SshLogger(SshClient *client, const QString &logFile) : m_logFile(logFile) {
        m_log.setFileName(logFile);
        m_log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        m_stream.setDevice(&m_log);

        connect(client, &SshClient::sshStateChanged,
                this, &SshLogger::logState);
        connect(client, &SshClient::sshError,
                this, &SshLogger::logError);
    }

    ~SshLogger() {
        m_log.close();
    }

private slots:
    void logState(SshClient::SshState state) {
        QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        m_stream << timestamp << " - State: " << state << "\n";
        m_stream.flush();
    }

    void logError() {
        QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        m_stream << timestamp << " - ERROR occurred\n";
        m_stream.flush();
    }

private:
    QString m_logFile;
    QFile m_log;
    QTextStream m_stream;
};

// Usage
SshClient *client = new SshClient("logged");
SshLogger *logger = new SshLogger(client, "/var/log/ssh-client.log");
```

## Debugging Tips

### Enable Qt Logging Categories

QtSsh uses Qt logging categories:

```cpp
// Enable all SSH logging
QLoggingCategory::setFilterRules("sshclient=true\n"
                                  "sshchannel=true\n"
                                  "logsshprocess=true\n"
                                  "logsshsftp=true\n"
                                  "logsshtunnelin=true\n"
                                  "logsshtunnelout=true");
```

### Monitor State Changes

```cpp
connect(client, &SshClient::sshStateChanged, [](SshClient::SshState state) {
    qDebug() << "Client state:" << state;
});

connect(channel, &SshChannel::stateChanged, [](SshChannel::ChannelState state) {
    qDebug() << "Channel state:" << state;
});
```

### Use libssh2 Error Strings

```cpp
// Helper function is provided
const char *errorStr = sshErrorToString(errorCode);
qDebug() << "libssh2 error:" << errorStr;
```

### Trace All Operations

```cpp
class OperationTracer {
public:
    static void trace(const QString &operation) {
        qDebug() << QTime::currentTime().toString()
                 << "[" << operation << "]";
    }
};

// Usage
OperationTracer::trace("Connecting to server");
client->connectToHost(user, host);

OperationTracer::trace("Executing command");
proc->runCommand(cmd);
```

## Best Practices

1. **Always connect error signals** before starting operations
2. **Check return values** from synchronous methods
3. **Implement timeouts** for all operations
4. **Log errors** for production debugging
5. **Provide user feedback** for long operations
6. **Handle disconnections gracefully** with cleanup
7. **Validate inputs** before SSH operations
8. **Test error paths** during development
9. **Use try-catch** for Qt exceptions in error handlers
10. **Document error conditions** in your code

## Testing Error Conditions

### Simulate Connection Failure

```cpp
// Use wrong port
client->connectToHost("user", "host", 2222);  // Wrong port
```

### Simulate Authentication Failure

```cpp
client->setPassphrase("wrong-password");
client->connectToHost("user", "host");
```

### Simulate Command Failure

```cpp
proc->runCommand("/nonexistent/command");
```

### Simulate File Transfer Failure

```cpp
// Try to upload non-existent file
sftp->send("/nonexistent/file.txt", "/remote/file.txt");

// Try to download from read-protected location
sftp->get("/root/protected.txt", "/local/file.txt");
```

## See Also

- [SshClient API](sshclient.md) - Connection management
- [Quick Start Guide](quickstart.md) - Basic usage examples
- [Installation Guide](installation.md) - Setup and building
