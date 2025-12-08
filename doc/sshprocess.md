# SshProcess API Documentation

## Overview

`SshProcess` provides remote command execution over SSH. It allows you to run commands on a remote server and retrieve their output, similar to executing commands locally with `QProcess`.

## Class Hierarchy

```
QObject
  └── SshChannel
        └── SshProcess
```

## Header

```cpp
#include "sshprocess.h"
```

## Creation

Channels are created through `SshClient::getChannel<T>()`:

```cpp
SshProcess *proc = client->getChannel<SshProcess>("processName");
```

## Public Methods

### runCommand

```cpp
void runCommand(const QString &cmd)
```

Executes a command on the remote server.

**Parameters:**
- `cmd`: Command string to execute

**Example:**
```cpp
SshProcess *proc = client->getChannel<SshProcess>("ls");
proc->runCommand("ls -la /home");
```

### result

```cpp
QByteArray result()
```

Returns the standard output from the executed command.

**Returns:** Command output as QByteArray

**Example:**
```cpp
connect(proc, &SshProcess::finished, [=]() {
    QByteArray output = proc->result();
    qDebug() << "Command output:" << output;
});
```

### errMsg

```cpp
QStringList errMsg()
```

Returns error messages from the command execution.

**Returns:** List of error messages

### isError

```cpp
bool isError()
```

Checks if an error occurred during command execution.

**Returns:** `true` if error occurred, `false` otherwise

### close

```cpp
void close() override
```

Closes the process channel.

## Signals

### finished

```cpp
void finished()
```

Emitted when the command execution completes successfully.

**Example:**
```cpp
connect(proc, &SshProcess::finished, [=]() {
    qDebug() << "Command finished successfully";
    qDebug() << proc->result();
});
```

### failed

```cpp
void failed()
```

Emitted when the command execution fails.

**Example:**
```cpp
connect(proc, &SshProcess::failed, [=]() {
    qDebug() << "Command failed";
    qDebug() << proc->errMsg();
});
```

## Usage Examples

### Basic Command Execution

```cpp
SshClient *client = new SshClient("example");
client->setPassphrase("password");

QObject::connect(client, &SshClient::sshReady, [=]() {
    SshProcess *proc = client->getChannel<SshProcess>("uptime");
    proc->runCommand("uptime");

    QObject::connect(proc, &SshProcess::finished, [=]() {
        qDebug() << "Server uptime:" << proc->result();
        client->disconnectFromHost();
    });
});

client->connectToHost("user", "server.com");
```

### Handling Command Errors

```cpp
SshProcess *proc = client->getChannel<SshProcess>("test");

connect(proc, &SshProcess::finished, [=]() {
    qDebug() << "Success:" << proc->result();
});

connect(proc, &SshProcess::failed, [=]() {
    qDebug() << "Failed with errors:";
    for (const QString &err : proc->errMsg()) {
        qDebug() << "  " << err;
    }
});

proc->runCommand("cat /nonexistent/file");
```

### Sequential Commands

```cpp
SshClient *client = new SshClient("sequential");

auto runCommands = [=]() {
    // First command
    SshProcess *proc1 = client->getChannel<SshProcess>("cmd1");
    proc1->runCommand("mkdir -p /tmp/test");

    connect(proc1, &SshProcess::finished, [=]() {
        qDebug() << "Directory created";

        // Second command
        SshProcess *proc2 = client->getChannel<SshProcess>("cmd2");
        proc2->runCommand("touch /tmp/test/file.txt");

        connect(proc2, &SshProcess::finished, [=]() {
            qDebug() << "File created";

            // Third command
            SshProcess *proc3 = client->getChannel<SshProcess>("cmd3");
            proc3->runCommand("ls -la /tmp/test");

            connect(proc3, &SshProcess::finished, [=]() {
                qDebug() << "Directory contents:" << proc3->result();
                client->disconnectFromHost();
            });
        });
    });
};

connect(client, &SshClient::sshReady, runCommands);
client->connectToHost("user", "server.com");
```

### Channel Reuse

```cpp
SshProcess *proc = client->getChannel<SshProcess>("reusable");

// First execution
proc->runCommand("df -h");
connect(proc, &SshProcess::finished, [=]() {
    qDebug() << "Disk usage:" << proc->result();

    // Reuse the same channel for another command
    proc->runCommand("free -m");
});
```

### Long-Running Commands

```cpp
SshProcess *proc = client->getChannel<SshProcess>("longrunning");

// Monitor state changes
connect(proc, &SshChannel::stateChanged, [](SshChannel::ChannelState state) {
    switch(state) {
        case SshChannel::Exec:
            qDebug() << "Command executing...";
            break;
        case SshChannel::Ready:
            qDebug() << "Command completed";
            break;
        default:
            break;
    }
});

proc->runCommand("sleep 10 && echo Done");

connect(proc, &SshProcess::finished, [=]() {
    qDebug() << proc->result();
});
```

### Parsing Command Output

```cpp
SshProcess *proc = client->getChannel<SshProcess>("parse");
proc->runCommand("ps aux");

connect(proc, &SshProcess::finished, [=]() {
    QString output = QString::fromUtf8(proc->result());
    QStringList lines = output.split('\n');

    qDebug() << "Running processes:";
    for (const QString &line : lines) {
        if (!line.isEmpty()) {
            qDebug() << line;
        }
    }
});
```

### Error Checking Pattern

```cpp
void executeRemoteCommand(SshClient *client, const QString &cmd) {
    SshProcess *proc = client->getChannel<SshProcess>("executor");

    connect(proc, &SshProcess::finished, [=]() {
        if (proc->isError()) {
            qWarning() << "Command failed:";
            for (const QString &err : proc->errMsg()) {
                qWarning() << "  " << err;
            }
        } else {
            qDebug() << "Success:" << proc->result();
        }
    });

    connect(proc, &SshProcess::failed, [=]() {
        qWarning() << "Execution failed";
    });

    proc->runCommand(cmd);
}
```

## Best Practices

1. **Signal Connection**: Always connect to `finished` and `failed` signals before calling `runCommand()`
2. **Channel Naming**: Use descriptive channel names for easier debugging
3. **Channel Reuse**: Reuse channels for multiple commands to reduce overhead
4. **Error Handling**: Check `isError()` even when `finished` is emitted
5. **Output Processing**: Use `QString::fromUtf8()` when converting output to QString
6. **Resource Cleanup**: Close channels when done or reuse them efficiently

## Common Patterns

### Command with Timeout

```cpp
SshProcess *proc = client->getChannel<SshProcess>("timeout");
QTimer *timeout = new QTimer();
timeout->setSingleShot(true);
timeout->setInterval(5000); // 5 seconds

connect(timeout, &QTimer::timeout, [=]() {
    qWarning() << "Command timed out";
    proc->close();
});

connect(proc, &SshProcess::finished, [=]() {
    timeout->stop();
    qDebug() << proc->result();
});

timeout->start();
proc->runCommand("your-command");
```

### Command Queue

```cpp
QQueue<QString> commands;
commands.enqueue("ls -la");
commands.enqueue("pwd");
commands.enqueue("whoami");

void processNext(SshClient *client, QQueue<QString> &queue) {
    if (queue.isEmpty()) {
        client->disconnectFromHost();
        return;
    }

    QString cmd = queue.dequeue();
    SshProcess *proc = client->getChannel<SshProcess>("queue");
    proc->runCommand(cmd);

    connect(proc, &SshProcess::finished, [&]() {
        qDebug() << cmd << "result:" << proc->result();
        processNext(client, queue);
    });
}
```

## Limitations

1. **Interactive Commands**: Commands requiring interactive input are not supported
2. **PTY Allocation**: No pseudo-terminal allocation (some programs may behave differently)
3. **Output Buffering**: All output is buffered; streaming is not available
4. **Exit Codes**: Exit codes are not directly exposed

## See Also

- [SshClient](sshclient.md) - Main client class
- [SshChannel](sshchannel.md) - Base channel class
- [Quick Start Guide](quickstart.md) - Getting started examples
