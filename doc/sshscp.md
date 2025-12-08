# SshScpGet and SshScpSend API Documentation

## Overview

`SshScpGet` and `SshScpSend` provide SCP (Secure Copy Protocol) functionality for secure file transfer. SCP is simpler than SFTP but offers good performance for straightforward file uploads and downloads.

## Class Hierarchy

```
QObject
  └── SshChannel
        ├── SshScpGet
        └── SshScpSend
```

## Headers

```cpp
#include "sshscpget.h"
#include "sshscpsend.h"
```

## Creation

Channels are created through `SshClient::getChannel<T>()`:

```cpp
SshScpGet *scpGet = client->getChannel<SshScpGet>("download");
SshScpSend *scpSend = client->getChannel<SshScpSend>("upload");
```

---

## SshScpSend (Upload Files)

### Methods

#### send

```cpp
void send(const QString &source, QString dest)
```

Uploads a local file to the remote server using SCP.

**Parameters:**
- `source`: Local file path
- `dest`: Remote destination path

**Example:**
```cpp
SshScpSend *scp = client->getChannel<SshScpSend>("upload");
scp->send("/local/file.txt", "/remote/path/file.txt");
```

#### close

```cpp
void close() override
```

Closes the SCP send channel.

### Signals

#### finished

```cpp
void finished()
```

Emitted when the file upload completes successfully.

#### failed

```cpp
void failed()
```

Emitted when the file upload fails.

#### progress

```cpp
void progress(qint64 tx, qint64 total)
```

Emitted periodically during upload to report progress.

**Parameters:**
- `tx`: Bytes transferred so far
- `total`: Total bytes to transfer

---

## SshScpGet (Download Files)

### Methods

#### get

```cpp
void get(const QString &source, const QString &dest)
```

Downloads a file from the remote server using SCP.

**Parameters:**
- `source`: Remote file path
- `dest`: Local destination path

**Example:**
```cpp
SshScpGet *scp = client->getChannel<SshScpGet>("download");
scp->get("/remote/file.txt", "/local/file.txt");
```

#### close

```cpp
void close() override
```

Closes the SCP get channel.

### Signals

#### finished

```cpp
void finished()
```

Emitted when the file download completes successfully.

#### failed

```cpp
void failed()
```

Emitted when the file download fails.

#### progress

```cpp
void progress(qint64 rx, qint64 total)
```

Emitted periodically during download to report progress.

**Parameters:**
- `rx`: Bytes received so far
- `total`: Total bytes to receive

---

## Usage Examples

### Upload File with Progress

```cpp
SshClient *client = new SshClient("upload");
client->setPassphrase("password");

connect(client, &SshClient::sshReady, [=]() {
    SshScpSend *scp = client->getChannel<SshScpSend>("upload");

    // Monitor progress
    connect(scp, &SshScpSend::progress, [](qint64 sent, qint64 total) {
        int percent = (sent * 100) / total;
        qDebug() << "Upload progress:" << percent << "%";
    });

    // Handle completion
    connect(scp, &SshScpSend::finished, [=]() {
        qDebug() << "Upload completed successfully";
        client->disconnectFromHost();
    });

    // Handle failure
    connect(scp, &SshScpSend::failed, [=]() {
        qDebug() << "Upload failed";
        client->disconnectFromHost();
    });

    // Start upload
    scp->send("/local/largefile.zip", "/remote/backup/largefile.zip");
});

client->connectToHost("user", "server.com");
```

### Download File with Progress

```cpp
SshClient *client = new SshClient("download");
client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");

connect(client, &SshClient::sshReady, [=]() {
    SshScpGet *scp = client->getChannel<SshScpGet>("download");

    // Monitor progress
    connect(scp, &SshScpGet::progress, [](qint64 received, qint64 total) {
        int percent = (received * 100) / total;
        qDebug() << "Download progress:" << percent << "%"
                 << "(" << received << "/" << total << "bytes)";
    });

    // Handle completion
    connect(scp, &SshScpGet::finished, [=]() {
        qDebug() << "Download completed";
        client->disconnectFromHost();
    });

    // Handle failure
    connect(scp, &SshScpGet::failed, [=]() {
        qWarning() << "Download failed";
        client->disconnectFromHost();
    });

    // Start download
    scp->get("/remote/database.sql", "/local/backup/database.sql");
});

client->connectToHost("user", "server.com");
```

### Upload Multiple Files

```cpp
void uploadFiles(SshClient *client, const QStringList &files, const QString &remotePath) {
    static int currentIndex = 0;

    if (currentIndex >= files.size()) {
        qDebug() << "All files uploaded";
        client->disconnectFromHost();
        return;
    }

    QString localFile = files[currentIndex];
    QFileInfo fileInfo(localFile);
    QString remoteFile = remotePath + "/" + fileInfo.fileName();

    SshScpSend *scp = client->getChannel<SshScpSend>("upload");

    connect(scp, &SshScpSend::finished, [=]() {
        qDebug() << "Uploaded:" << fileInfo.fileName();
        currentIndex++;
        uploadFiles(client, files, remotePath);
    });

    connect(scp, &SshScpSend::failed, [=]() {
        qWarning() << "Failed to upload:" << fileInfo.fileName();
        currentIndex++;
        uploadFiles(client, files, remotePath);
    });

    qDebug() << "Uploading:" << fileInfo.fileName();
    scp->send(localFile, remoteFile);
}

// Usage
connect(client, &SshClient::sshReady, [=]() {
    QStringList files = {
        "/local/file1.txt",
        "/local/file2.txt",
        "/local/file3.txt"
    };
    uploadFiles(client, files, "/remote/uploads");
});
```

### Download with Progress Bar (Qt Widgets)

```cpp
#include <QProgressDialog>

void downloadFileWithProgressBar(SshClient *client, const QString &remote, const QString &local) {
    SshScpGet *scp = client->getChannel<SshScpGet>("download");

    QProgressDialog *progress = new QProgressDialog("Downloading file...", "Cancel", 0, 100);
    progress->setWindowModality(Qt::WindowModal);
    progress->show();

    connect(scp, &SshScpGet::progress, [=](qint64 received, qint64 total) {
        int percent = (received * 100) / total;
        progress->setValue(percent);

        // Allow cancellation
        if (progress->wasCanceled()) {
            scp->close();
        }
    });

    connect(scp, &SshScpGet::finished, [=]() {
        progress->setValue(100);
        progress->close();
        qDebug() << "Download complete";
        delete progress;
    });

    connect(scp, &SshScpGet::failed, [=]() {
        progress->close();
        qWarning() << "Download failed";
        delete progress;
    });

    scp->get(remote, local);
}
```

### Backup with SCP

```cpp
class BackupManager : public QObject {
    Q_OBJECT
public:
    BackupManager(SshClient *client) : m_client(client) {}

    void backupFiles(const QStringList &remoteFiles, const QString &localBackupDir) {
        m_remoteFiles = remoteFiles;
        m_localBackupDir = localBackupDir;
        m_currentIndex = 0;

        QDir().mkpath(localBackupDir);
        downloadNext();
    }

private slots:
    void downloadNext() {
        if (m_currentIndex >= m_remoteFiles.size()) {
            qDebug() << "Backup completed";
            emit backupComplete();
            return;
        }

        QString remoteFile = m_remoteFiles[m_currentIndex];
        QFileInfo info(remoteFile);
        QString localFile = m_localBackupDir + "/" + info.fileName();

        SshScpGet *scp = m_client->getChannel<SshScpGet>("backup");

        connect(scp, &SshScpGet::finished, this, [=]() {
            qDebug() << "Backed up:" << info.fileName();
            m_currentIndex++;
            downloadNext();
        });

        connect(scp, &SshScpGet::failed, this, [=]() {
            qWarning() << "Failed to backup:" << info.fileName();
            m_currentIndex++;
            downloadNext();
        });

        qDebug() << "Backing up:" << info.fileName();
        scp->get(remoteFile, localFile);
    }

signals:
    void backupComplete();

private:
    SshClient *m_client;
    QStringList m_remoteFiles;
    QString m_localBackupDir;
    int m_currentIndex;
};
```

### Speed Measurement

```cpp
class TransferMonitor : public QObject {
    Q_OBJECT
public:
    TransferMonitor(QObject *parent = nullptr) : QObject(parent) {
        m_timer.start();
    }

    void updateProgress(qint64 bytes, qint64 total) {
        qint64 elapsed = m_timer.elapsed();
        if (elapsed > 0) {
            double speed = (bytes * 1000.0) / elapsed; // bytes per second
            double mbps = speed / (1024 * 1024); // MB/s

            int percent = (bytes * 100) / total;
            qint64 remaining = total - bytes;
            qint64 estimatedTime = (remaining * elapsed) / bytes;

            qDebug() << QString("Progress: %1% | Speed: %2 MB/s | ETA: %3s")
                        .arg(percent)
                        .arg(mbps, 0, 'f', 2)
                        .arg(estimatedTime / 1000);
        }
    }

private:
    QElapsedTimer m_timer;
};

// Usage
SshScpGet *scp = client->getChannel<SshScpGet>("fast");
TransferMonitor *monitor = new TransferMonitor();

connect(scp, &SshScpGet::progress, monitor, &TransferMonitor::updateProgress);
scp->get("/remote/largefile.iso", "/local/largefile.iso");
```

## SCP vs SFTP Comparison

| Feature | SCP | SFTP |
|---------|-----|------|
| **Protocol** | Simple file transfer | Full file system protocol |
| **Directory Operations** | No | Yes (mkdir, readdir, etc.) |
| **File Listing** | No | Yes |
| **Resume Support** | No | Possible |
| **Progress Reporting** | Yes | Limited |
| **Performance** | Generally faster | Slightly slower |
| **Use Case** | Simple file transfers | Complex file operations |

## When to Use SCP

Use SCP when you need:
- Simple file upload or download
- Progress monitoring
- Maximum transfer speed
- Minimal overhead

Use SFTP when you need:
- Directory listing and operations
- File attribute queries
- Multiple file operations
- Directory synchronization

## Best Practices

1. **Signal Connection**: Connect to signals before starting transfer
2. **Progress Monitoring**: Use `progress` signal for user feedback
3. **Error Handling**: Always connect to both `finished` and `failed`
4. **Resource Cleanup**: Close channels or disconnect client when done
5. **Path Validation**: Validate file paths before transfer
6. **Large Files**: SCP with progress monitoring is ideal for large files

## Common Patterns

### Transfer with Retry

```cpp
void transferWithRetry(SshScpSend *scp, const QString &local, const QString &remote, int maxRetries = 3) {
    static int retryCount = 0;

    connect(scp, &SshScpSend::finished, [=]() {
        qDebug() << "Transfer successful";
        retryCount = 0;
    });

    connect(scp, &SshScpSend::failed, [=]() {
        retryCount++;
        if (retryCount < maxRetries) {
            qWarning() << "Transfer failed, retrying..." << retryCount << "/" << maxRetries;
            QTimer::singleShot(1000, [=]() {
                scp->send(local, remote);
            });
        } else {
            qCritical() << "Transfer failed after" << maxRetries << "attempts";
            retryCount = 0;
        }
    });

    scp->send(local, remote);
}
```

### Verify File Size After Transfer

```cpp
void verifyTransfer(SshScpSend *scp, const QString &local, const QString &remote) {
    QFileInfo localInfo(local);
    qint64 localSize = localInfo.size();

    connect(scp, &SshScpSend::finished, [=]() {
        // Use SFTP to verify remote file size
        SshSFtp *sftp = scp->sshClient()->getChannel<SshSFtp>("verify");
        qint64 remoteSize = sftp->filesize(remote);

        if (remoteSize == localSize) {
            qDebug() << "Transfer verified: sizes match";
        } else {
            qWarning() << "Transfer verification failed: size mismatch";
        }
    });

    scp->send(local, remote);
}
```

## Limitations

1. **Single File**: Can only transfer one file per operation
2. **No Resume**: Cannot resume interrupted transfers
3. **No Directory Transfer**: Cannot directly transfer directories (must transfer files individually)
4. **Limited Metadata**: File permissions and timestamps may not be preserved

## See Also

- [SshClient](sshclient.md) - Main client class
- [SshChannel](sshchannel.md) - Base channel class
- [SshSFtp](sshsftp.md) - Alternative file transfer with more features
- [Quick Start Guide](quickstart.md) - Getting started examples
