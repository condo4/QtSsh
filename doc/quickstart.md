# QtSsh Quick Start Guide

## Introduction

This guide will help you get started with QtSsh quickly. We'll cover the basic operations: connecting to an SSH server, executing commands, transferring files, and setting up tunnels.

## Prerequisites

- QtSsh installed and integrated into your project (see [Installation Guide](installation.md))
- An SSH server to connect to
- Basic knowledge of Qt and C++

## Your First SSH Connection

Let's create a simple application that connects to an SSH server and executes a command.

### Step 1: Include Headers

```cpp
#include <QCoreApplication>
#include <QDebug>
#include "sshclient.h"
#include "sshprocess.h"
```

### Step 2: Create SSH Client

```cpp
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Create SSH client
    SshClient *client = new SshClient("myConnection");

    // Set authentication (choose one method)
    client->setPassphrase("your-password");  // Password authentication
    // OR
    // client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");  // Key authentication

    return app.exec();
}
```

### Step 3: Handle Connection Events

```cpp
// Called when connected and ready
QObject::connect(client, &SshClient::sshReady, [=]() {
    qDebug() << "Connected to SSH server!";

    // Execute a command
    SshProcess *proc = client->getChannel<SshProcess>("myCommand");
    proc->runCommand("ls -la");

    // Handle command completion
    QObject::connect(proc, &SshProcess::finished, [=]() {
        qDebug() << "Command output:";
        qDebug() << proc->result();

        // Disconnect when done
        client->disconnectFromHost();
    });
});

// Handle errors
QObject::connect(client, &SshClient::sshError, [=]() {
    qDebug() << "Connection error!";
    app.quit();
});

// Handle disconnection
QObject::connect(client, &SshClient::sshDisconnected, [=]() {
    qDebug() << "Disconnected";
    app.quit();
});
```

### Step 4: Connect to Server

```cpp
// Connect to SSH server
client->connectToHost("username", "server.example.com", 22);
```

### Complete Example

```cpp
#include <QCoreApplication>
#include <QDebug>
#include "sshclient.h"
#include "sshprocess.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SshClient *client = new SshClient("example");
    client->setPassphrase("password");

    QObject::connect(client, &SshClient::sshReady, [=]() {
        qDebug() << "Connected!";

        SshProcess *proc = client->getChannel<SshProcess>("ls");
        proc->runCommand("ls -la");

        QObject::connect(proc, &SshProcess::finished, [=]() {
            qDebug() << "Output:" << proc->result();
            client->disconnectFromHost();
        });
    });

    QObject::connect(client, &SshClient::sshError, [&]() {
        qDebug() << "Error!";
        app.quit();
    });

    QObject::connect(client, &SshClient::sshDisconnected, [&]() {
        qDebug() << "Disconnected";
        app.quit();
    });

    client->connectToHost("user", "server.com");

    return app.exec();
}
```

## Common Tasks

### 1. Execute Remote Command

```cpp
void executeCommand(SshClient *client, const QString &command) {
    SshProcess *proc = client->getChannel<SshProcess>("cmd");

    QObject::connect(proc, &SshProcess::finished, [=]() {
        qDebug() << "Result:" << proc->result();
    });

    QObject::connect(proc, &SshProcess::failed, [=]() {
        qDebug() << "Failed:" << proc->errMsg();
    });

    proc->runCommand(command);
}

// Usage
connect(client, &SshClient::sshReady, [=]() {
    executeCommand(client, "uptime");
});
```

### 2. Upload File (SFTP)

```cpp
void uploadFile(SshClient *client, const QString &local, const QString &remote) {
    SshSFtp *sftp = client->getChannel<SshSFtp>("upload");

    QString error = sftp->send(local, remote);

    if (error.isEmpty()) {
        qDebug() << "File uploaded successfully";
    } else {
        qDebug() << "Upload failed:" << error;
    }
}

// Usage
connect(client, &SshClient::sshReady, [=]() {
    uploadFile(client, "/local/file.txt", "/remote/file.txt");
});
```

### 3. Download File (SFTP)

```cpp
void downloadFile(SshClient *client, const QString &remote, const QString &local) {
    SshSFtp *sftp = client->getChannel<SshSFtp>("download");

    bool success = sftp->get(remote, local, true);

    if (success) {
        qDebug() << "File downloaded successfully";
    } else {
        qDebug() << "Download failed:" << sftp->errMsg();
    }
}

// Usage
connect(client, &SshClient::sshReady, [=]() {
    downloadFile(client, "/remote/data.zip", "/local/data.zip");
});
```

### 4. Upload File with Progress (SCP)

```cpp
void uploadWithProgress(SshClient *client, const QString &local, const QString &remote) {
    SshScpSend *scp = client->getChannel<SshScpSend>("upload");

    // Monitor progress
    QObject::connect(scp, &SshScpSend::progress, [](qint64 sent, qint64 total) {
        int percent = (sent * 100) / total;
        qDebug() << "Upload:" << percent << "%";
    });

    // Handle completion
    QObject::connect(scp, &SshScpSend::finished, []() {
        qDebug() << "Upload complete!";
    });

    // Start upload
    scp->send(local, remote);
}

// Usage
connect(client, &SshClient::sshReady, [=]() {
    uploadWithProgress(client, "/local/big.zip", "/remote/big.zip");
});
```

### 5. Create SSH Tunnel

```cpp
void createTunnel(SshClient *client) {
    SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("mysql");

    // Forward local port 3306 to remote MySQL server
    tunnel->listen(3306, "mysql.internal.network");

    qDebug() << "Tunnel created! Connect to localhost:3306";
}

// Usage
connect(client, &SshClient::sshReady, [=]() {
    createTunnel(client);
    // Keep connection alive, don't disconnect
});
```

## Complete Working Examples

### Example 1: Remote System Information

```cpp
#include <QCoreApplication>
#include <QDebug>
#include "sshclient.h"
#include "sshprocess.h"

class SystemInfo : public QObject {
    Q_OBJECT
public:
    SystemInfo(const QString &host, const QString &user, const QString &pass) {
        m_client = new SshClient("sysinfo", this);
        m_client->setPassphrase(pass);

        connect(m_client, &SshClient::sshReady, this, &SystemInfo::onConnected);
        connect(m_client, &SshClient::sshError, this, &SystemInfo::onError);

        m_client->connectToHost(user, host);
    }

private slots:
    void onConnected() {
        qDebug() << "=== System Information ===\n";
        runCommand("Hostname", "hostname");
    }

    void runCommand(const QString &label, const QString &cmd) {
        SshProcess *proc = m_client->getChannel<SshProcess>(label);

        connect(proc, &SshProcess::finished, [=]() {
            qDebug() << label << ":" << proc->result().trimmed();

            // Run next command
            if (label == "Hostname") {
                runCommand("OS", "uname -a");
            } else if (label == "OS") {
                runCommand("Uptime", "uptime");
            } else if (label == "Uptime") {
                runCommand("Memory", "free -h");
            } else {
                qDebug() << "\n=== Done ===";
                m_client->disconnectFromHost();
                qApp->quit();
            }
        });

        proc->runCommand(cmd);
    }

    void onError() {
        qDebug() << "Connection failed!";
        qApp->quit();
    }

private:
    SshClient *m_client;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    new SystemInfo("server.com", "user", "password");

    return app.exec();
}

#include "main.moc"
```

### Example 2: Batch File Transfer

```cpp
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include "sshclient.h"
#include "sshsftp.h"

class FileUploader : public QObject {
    Q_OBJECT
public:
    FileUploader(SshClient *client, const QStringList &files, const QString &remotePath)
        : m_client(client), m_files(files), m_remotePath(remotePath), m_index(0) {
        uploadNext();
    }

private:
    void uploadNext() {
        if (m_index >= m_files.size()) {
            qDebug() << "All files uploaded!";
            m_client->disconnectFromHost();
            return;
        }

        QString localFile = m_files[m_index];
        QFileInfo info(localFile);
        QString remoteFile = m_remotePath + "/" + info.fileName();

        qDebug() << "Uploading:" << info.fileName();

        SshSFtp *sftp = m_client->getChannel<SshSFtp>("upload");
        QString error = sftp->send(localFile, remoteFile);

        if (error.isEmpty()) {
            qDebug() << "  Success!";
        } else {
            qDebug() << "  Failed:" << error;
        }

        m_index++;
        uploadNext();
    }

private:
    SshClient *m_client;
    QStringList m_files;
    QString m_remotePath;
    int m_index;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SshClient *client = new SshClient("uploader");
    client->setPassphrase("password");

    QObject::connect(client, &SshClient::sshReady, [=]() {
        QStringList files = {
            "/local/doc1.pdf",
            "/local/doc2.pdf",
            "/local/doc3.pdf"
        };
        new FileUploader(client, files, "/remote/documents");
    });

    QObject::connect(client, &SshClient::sshDisconnected, [&]() {
        app.quit();
    });

    client->connectToHost("user", "server.com");

    return app.exec();
}

#include "main.moc"
```

### Example 3: Database Tunnel

```cpp
#include <QCoreApplication>
#include <QDebug>
#include "sshclient.h"
#include "sshtunnelout.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SshClient *client = new SshClient("db-tunnel");
    client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");

    QObject::connect(client, &SshClient::sshReady, [=]() {
        qDebug() << "SSH connected, creating tunnel...";

        SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("postgres");

        QObject::connect(tunnel, &SshTunnelOut::connectionChanged, [](int count) {
            qDebug() << "Database connections:" << count;
        });

        // Forward local port 5432 to PostgreSQL server
        tunnel->listen(5432, "postgres.internal.network");

        qDebug() << "Tunnel ready!";
        qDebug() << "Connect to: localhost:5432";
        qDebug() << "Press Ctrl+C to stop";
    });

    QObject::connect(client, &SshClient::sshError, [&]() {
        qDebug() << "Connection error!";
        app.quit();
    });

    client->connectToHost("user", "gateway.company.com");

    return app.exec();
}
```

## Tips and Best Practices

### 1. Always Handle Signals

Always connect to error and disconnection signals:

```cpp
connect(client, &SshClient::sshError, [&]() {
    qDebug() << "Error occurred";
    app.quit();
});

connect(client, &SshClient::sshDisconnected, [&]() {
    qDebug() << "Disconnected";
    app.quit();
});
```

### 2. Reuse Channels

Channels can be reused for multiple operations:

```cpp
SshProcess *proc = client->getChannel<SshProcess>("worker");

// First command
proc->runCommand("ls");
connect(proc, &SshProcess::finished, [=]() {
    // Second command
    proc->runCommand("pwd");
});
```

### 3. Use Descriptive Names

Give channels meaningful names for debugging:

```cpp
SshProcess *backup = client->getChannel<SshProcess>("backup-db");
SshProcess *deploy = client->getChannel<SshProcess>("deploy-app");
SshSFtp *upload = client->getChannel<SshSFtp>("upload-files");
```

### 4. Check Return Values

Always check return values and error states:

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("upload");
QString error = sftp->send(local, remote);

if (!error.isEmpty()) {
    qDebug() << "Upload failed:" << error;
    if (sftp->isError()) {
        qDebug() << "Errors:" << sftp->errMsg();
    }
}
```

### 5. Wait for State if Needed

Use `waitForState()` for synchronous-style code:

```cpp
client->connectToHost("user", "host.com");

if (client->waitForState(SshClient::Ready)) {
    // Connected, do work
    SshProcess *proc = client->getChannel<SshProcess>("cmd");
    proc->runCommand("ls");
    proc->waitForState(SshChannel::Ready);
    qDebug() << proc->result();
} else {
    qDebug() << "Connection timeout";
}
```

## Common Patterns

### Pattern 1: Command with Callback

```cpp
void runCommand(SshClient *client, const QString &cmd,
                std::function<void(QByteArray)> callback) {
    SshProcess *proc = client->getChannel<SshProcess>("runner");

    QObject::connect(proc, &SshProcess::finished, [=]() {
        callback(proc->result());
    });

    proc->runCommand(cmd);
}

// Usage
runCommand(client, "hostname", [](QByteArray output) {
    qDebug() << "Hostname:" << output;
});
```

### Pattern 2: Sequential Operations

```cpp
class TaskRunner : public QObject {
    Q_OBJECT
public:
    TaskRunner(SshClient *client, QStringList commands)
        : m_client(client), m_commands(commands), m_index(0) {
        runNext();
    }

private:
    void runNext() {
        if (m_index >= m_commands.size()) {
            emit allDone();
            return;
        }

        QString cmd = m_commands[m_index];
        SshProcess *proc = m_client->getChannel<SshProcess>("task");

        connect(proc, &SshProcess::finished, [=]() {
            qDebug() << "Done:" << cmd;
            m_index++;
            runNext();
        });

        proc->runCommand(cmd);
    }

signals:
    void allDone();

private:
    SshClient *m_client;
    QStringList m_commands;
    int m_index;
};
```

## Next Steps

- Read detailed [API Documentation](sshclient.md)
- Learn about [Error Handling](errors.md)
- Check out the test applications in the `test/` directory
- Explore advanced features like [SSH Tunneling](tunneling.md)

## Troubleshooting

**Problem:** Connection hangs

**Solution:** Check firewall settings and ensure port 22 is accessible

---

**Problem:** Authentication fails

**Solution:** Verify credentials and authentication method (password vs key)

---

**Problem:** Commands don't execute

**Solution:** Ensure you wait for `sshReady` signal before creating channels

---

**Problem:** Application doesn't exit

**Solution:** Make sure to call `app.quit()` in disconnection handler

## Need Help?

- See [Error Handling Guide](errors.md) for debugging tips
- Check [Installation Guide](installation.md) for build issues
- Review example applications in `test/` directory
- Consult libssh2 documentation for underlying SSH functionality
