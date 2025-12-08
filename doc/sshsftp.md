# SshSFtp API Documentation

## Overview

`SshSFtp` provides SFTP (SSH File Transfer Protocol) functionality for secure file transfer and remote file system operations. It supports file upload/download, directory operations, and file attribute queries.

## Class Hierarchy

```
QObject
  └── SshChannel
        └── SshSFtp
```

## Header

```cpp
#include "sshsftp.h"
```

## Creation

Channels are created through `SshClient::getChannel<T>()`:

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("sftpName");
```

## Public Methods

### File Transfer

#### send

```cpp
QString send(const QString &source, QString dest)
```

Uploads a local file to the remote server.

**Parameters:**
- `source`: Local file path
- `dest`: Remote destination path

**Returns:** Empty string on success, error message on failure

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("upload");
QString error = sftp->send("/local/file.txt", "/remote/path/file.txt");
if (!error.isEmpty()) {
    qDebug() << "Upload failed:" << error;
}
```

#### get

```cpp
bool get(const QString &source, QString dest, bool override = false)
```

Downloads a file from the remote server.

**Parameters:**
- `source`: Remote file path
- `dest`: Local destination path
- `override`: If true, overwrite existing local file

**Returns:** `true` on success, `false` on failure

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("download");
bool success = sftp->get("/remote/file.txt", "/local/file.txt", true);
if (!success) {
    qDebug() << "Download failed:" << sftp->errMsg();
}
```

### Directory Operations

#### mkdir

```cpp
int mkdir(const QString &dest, int mode = 0755)
```

Creates a directory on the remote server.

**Parameters:**
- `dest`: Remote directory path
- `mode`: Unix permissions (default: 0755)

**Returns:** 0 on success, error code on failure

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("mkdir");
int result = sftp->mkdir("/remote/newdir", 0755);
if (result != 0) {
    qDebug() << "Failed to create directory";
}
```

#### mkpath

```cpp
int mkpath(const QString &dest)
```

Creates a directory path recursively (similar to `mkdir -p`).

**Parameters:**
- `dest`: Remote directory path

**Returns:** 0 on success, error code on failure

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("mkpath");
sftp->mkpath("/remote/path/to/deep/directory");
```

#### readdir

```cpp
QStringList readdir(const QString &d)
```

Lists files in a remote directory.

**Parameters:**
- `d`: Remote directory path

**Returns:** List of file/directory names

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("ls");
QStringList files = sftp->readdir("/remote/path");
for (const QString &file : files) {
    qDebug() << file;
}
```

### File Operations

#### unlink

```cpp
bool unlink(const QString &d)
```

Deletes a file on the remote server.

**Parameters:**
- `d`: Remote file path

**Returns:** `true` on success, `false` on failure

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("delete");
if (sftp->unlink("/remote/file.txt")) {
    qDebug() << "File deleted";
} else {
    qDebug() << "Failed to delete file";
}
```

### File Attributes

#### isDir

```cpp
bool isDir(const QString &d)
```

Checks if a path is a directory.

**Parameters:**
- `d`: Remote path

**Returns:** `true` if directory, `false` otherwise

#### isFile

```cpp
bool isFile(const QString &d)
```

Checks if a path is a regular file.

**Parameters:**
- `d`: Remote path

**Returns:** `true` if file, `false` otherwise

#### filesize

```cpp
quint64 filesize(const QString &d)
```

Gets the size of a remote file.

**Parameters:**
- `d`: Remote file path

**Returns:** File size in bytes

**Example:**
```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("info");
if (sftp->isFile("/remote/file.txt")) {
    quint64 size = sftp->filesize("/remote/file.txt");
    qDebug() << "File size:" << size << "bytes";
}
```

### Error Handling

#### isError

```cpp
bool isError()
```

Checks if an error occurred during the last operation.

**Returns:** `true` if error occurred

#### errMsg

```cpp
QStringList errMsg()
```

Gets error messages from the last operation.

**Returns:** List of error messages

### Other Methods

#### close

```cpp
void close() override
```

Closes the SFTP channel.

#### getSftpSession

```cpp
LIBSSH2_SFTP *getSftpSession() const
```

Returns the underlying libssh2 SFTP session (for advanced use).

## Signals

### sendEvent

```cpp
void sendEvent()
```

Emitted during send operations.

### cmdEvent

```cpp
void cmdEvent()
```

Emitted during command processing.

## Usage Examples

### Upload File

```cpp
SshClient *client = new SshClient("upload");
client->setPassphrase("password");

connect(client, &SshClient::sshReady, [=]() {
    SshSFtp *sftp = client->getChannel<SshSFtp>("upload");

    QString error = sftp->send("/local/document.pdf", "/remote/docs/document.pdf");

    if (error.isEmpty()) {
        qDebug() << "File uploaded successfully";
    } else {
        qDebug() << "Upload error:" << error;
    }

    client->disconnectFromHost();
});

client->connectToHost("user", "server.com");
```

### Download File

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("download");

bool success = sftp->get("/remote/data.zip", "/local/downloads/data.zip", true);

if (success) {
    qDebug() << "Downloaded successfully";
} else {
    qDebug() << "Download failed:";
    for (const QString &err : sftp->errMsg()) {
        qDebug() << "  " << err;
    }
}
```

### List Remote Directory

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("list");
QString remotePath = "/home/user/documents";

QStringList files = sftp->readdir(remotePath);

qDebug() << "Contents of" << remotePath << ":";
for (const QString &file : files) {
    QString fullPath = remotePath + "/" + file;

    if (sftp->isDir(fullPath)) {
        qDebug() << "[DIR] " << file;
    } else if (sftp->isFile(fullPath)) {
        quint64 size = sftp->filesize(fullPath);
        qDebug() << "[FILE]" << file << "(" << size << "bytes)";
    }
}
```

### Create Directory Structure

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("mkdir");

// Create nested directories
int result = sftp->mkpath("/remote/projects/2024/data");

if (result == 0) {
    qDebug() << "Directory structure created";
} else {
    qDebug() << "Failed to create directories";
}
```

### Synchronize Local Directory to Remote

```cpp
void uploadDirectory(SshSFtp *sftp, const QString &localPath, const QString &remotePath) {
    QDir localDir(localPath);

    // Create remote directory
    sftp->mkpath(remotePath);

    // Upload all files
    QFileInfoList entries = localDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo &entry : entries) {
        QString remoteDest = remotePath + "/" + entry.fileName();

        if (entry.isDir()) {
            // Recursively upload subdirectory
            uploadDirectory(sftp, entry.absoluteFilePath(), remoteDest);
        } else {
            // Upload file
            QString error = sftp->send(entry.absoluteFilePath(), remoteDest);
            if (error.isEmpty()) {
                qDebug() << "Uploaded:" << entry.fileName();
            } else {
                qWarning() << "Failed to upload" << entry.fileName() << ":" << error;
            }
        }
    }
}

// Usage
SshSFtp *sftp = client->getChannel<SshSFtp>("sync");
uploadDirectory(sftp, "/local/project", "/remote/backup/project");
```

### Delete Remote Files by Pattern

```cpp
void deleteFilesByExtension(SshSFtp *sftp, const QString &remotePath, const QString &extension) {
    QStringList files = sftp->readdir(remotePath);

    for (const QString &file : files) {
        if (file.endsWith(extension)) {
            QString fullPath = remotePath + "/" + file;
            if (sftp->isFile(fullPath)) {
                if (sftp->unlink(fullPath)) {
                    qDebug() << "Deleted:" << file;
                } else {
                    qWarning() << "Failed to delete:" << file;
                }
            }
        }
    }
}

// Usage: Delete all .tmp files
SshSFtp *sftp = client->getChannel<SshSFtp>("cleanup");
deleteFilesByExtension(sftp, "/remote/temp", ".tmp");
```

### Check Remote File Before Download

```cpp
SshSFtp *sftp = client->getChannel<SshSFtp>("check");
QString remoteFile = "/remote/data.zip";

if (sftp->isFile(remoteFile)) {
    quint64 size = sftp->filesize(remoteFile);

    qDebug() << "Remote file exists, size:" << size << "bytes";

    // Check if we have enough local space
    QStorageInfo storage(QDir::homePath());
    if (storage.bytesAvailable() > size) {
        bool success = sftp->get(remoteFile, "/local/data.zip", true);
        if (success) {
            qDebug() << "Download complete";
        }
    } else {
        qWarning() << "Insufficient local storage space";
    }
} else {
    qWarning() << "Remote file does not exist";
}
```

### Backup Remote Directory

```cpp
void backupRemoteDirectory(SshSFtp *sftp, const QString &remotePath, const QString &localPath) {
    QDir().mkpath(localPath);

    QStringList entries = sftp->readdir(remotePath);

    for (const QString &entry : entries) {
        if (entry == "." || entry == "..") continue;

        QString remoteFullPath = remotePath + "/" + entry;
        QString localFullPath = localPath + "/" + entry;

        if (sftp->isDir(remoteFullPath)) {
            // Recursively backup subdirectory
            backupRemoteDirectory(sftp, remoteFullPath, localFullPath);
        } else if (sftp->isFile(remoteFullPath)) {
            // Download file
            if (sftp->get(remoteFullPath, localFullPath, true)) {
                qDebug() << "Backed up:" << entry;
            } else {
                qWarning() << "Failed to backup:" << entry;
            }
        }
    }
}

// Usage
SshSFtp *sftp = client->getChannel<SshSFtp>("backup");
backupRemoteDirectory(sftp, "/remote/important", "/local/backups/important");
```

## Best Practices

1. **Error Checking**: Always check return values and `isError()`
2. **Path Validation**: Validate paths before operations
3. **Permissions**: Set appropriate file/directory permissions
4. **Channel Reuse**: Reuse SFTP channels for multiple operations
5. **Large Files**: Consider progress monitoring for large file transfers
6. **Cleanup**: Use `unlink()` to clean up temporary files

## Common Patterns

### Safe File Upload

```cpp
QString safeUpload(SshSFtp *sftp, const QString &local, const QString &remote) {
    // Check local file exists
    if (!QFile::exists(local)) {
        return "Local file does not exist";
    }

    // Create remote directory if needed
    QFileInfo remoteInfo(remote);
    sftp->mkpath(remoteInfo.path());

    // Upload
    QString error = sftp->send(local, remote);
    return error;
}
```

### Safe File Download

```cpp
bool safeDownload(SshSFtp *sftp, const QString &remote, const QString &local) {
    // Check remote file exists
    if (!sftp->isFile(remote)) {
        qWarning() << "Remote file does not exist";
        return false;
    }

    // Create local directory if needed
    QFileInfo localInfo(local);
    QDir().mkpath(localInfo.path());

    // Download
    return sftp->get(remote, local, true);
}
```

## Limitations

1. **Symbolic Links**: Limited support for symbolic links
2. **File Attributes**: Some Unix file attributes may not be fully supported
3. **Performance**: Large directories may take time to list
4. **Concurrency**: Operations are sequential within a channel

## See Also

- [SshClient](sshclient.md) - Main client class
- [SshChannel](sshchannel.md) - Base channel class
- [SshScpGet/SshScpSend](sshscp.md) - Alternative file transfer using SCP
- [Quick Start Guide](quickstart.md) - Getting started examples
