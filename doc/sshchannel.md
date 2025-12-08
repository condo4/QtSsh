# SshChannel API Documentation

## Overview

`SshChannel` is the abstract base class for all SSH channel types in QtSsh. It provides common functionality and state management for SSH operations. All specific channel types (process execution, file transfer, tunneling) inherit from this class.

## Class Hierarchy

```
QObject
  └── SshChannel (abstract)
        ├── SshProcess
        ├── SshSFtp
        ├── SshScpGet
        ├── SshScpSend
        ├── SshTunnelIn
        └── SshTunnelOut
```

## Header

```cpp
#include "sshchannel.h"
```

## Constructor

```cpp
explicit SshChannel(QString name, SshClient *client)
```

This constructor is protected and called by derived classes.

**Parameters:**
- `name`: Unique identifier for the channel
- `client`: Pointer to the parent SshClient

## Methods

### name

```cpp
QString name() const
```

Returns the name of this channel.

**Returns:** Channel name

### close

```cpp
virtual void close() = 0
```

Pure virtual function to close the channel. Must be implemented by derived classes.

### channelState

```cpp
ChannelState channelState() const
```

Returns the current state of the channel.

**Returns:** Current channel state

### setChannelState

```cpp
void setChannelState(const ChannelState &channelState)
```

Sets the channel state (typically used internally).

**Parameters:**
- `channelState`: New channel state

### waitForState

```cpp
bool waitForState(SshChannel::ChannelState state)
```

Blocks until the channel reaches the specified state.

**Parameters:**
- `state`: Target state to wait for

**Returns:** `true` if state was reached, `false` on timeout

**Example:**
```cpp
SshProcess *proc = client->getChannel<SshProcess>("myProc");
proc->runCommand("ls");
if (proc->waitForState(SshChannel::Ready)) {
    // Command completed
}
```

### sshClient

```cpp
SshClient *sshClient() const
```

Returns a pointer to the parent SSH client.

**Returns:** Pointer to SshClient

## Channel States

The `ChannelState` enum defines the lifecycle of a channel:

```cpp
enum ChannelState {
    Openning,     // Channel is being opened
    Exec,         // Executing operation
    Ready,        // Operation complete, ready for new operations
    Close,        // Channel is closing
    WaitClose,    // Waiting for close confirmation
    Freeing,      // Freeing resources
    Free,         // Channel is freed
    Error         // Error occurred
}
```

## Signals

### stateChanged

```cpp
void stateChanged(ChannelState state)
```

Emitted when the channel state changes.

**Parameters:**
- `state`: New channel state

**Example:**
```cpp
connect(channel, &SshChannel::stateChanged, [](SshChannel::ChannelState state) {
    qDebug() << "Channel state:" << state;
});
```

## Protected Slots

### sshDataReceived

```cpp
virtual void sshDataReceived()
```

Virtual slot called when SSH data is received. Derived classes override this to handle incoming data.

## Usage Pattern

Channels are typically created using `SshClient::getChannel<T>()`:

```cpp
// Create or get a process channel
SshProcess *proc = client->getChannel<SshProcess>("process1");

// Create or get an SFTP channel
SshSFtp *sftp = client->getChannel<SshSFtp>("sftp1");
```

## Channel Lifecycle

1. **Creation**: Channel is created via `SshClient::getChannel<T>()`
2. **Opening**: Channel state is `Openning`
3. **Operation**: State changes to `Exec` during operation
4. **Ready**: State becomes `Ready` when operation completes
5. **Reuse**: Channel can be reused for multiple operations
6. **Close**: Call `close()` to close the channel
7. **Cleanup**: State transitions through `Close` → `WaitClose` → `Freeing` → `Free`

## Example: Monitoring Channel State

```cpp
SshClient *client = new SshClient("myClient");
client->connectToHost("user", "host.com");
client->waitForState(SshClient::Ready);

// Get a channel
SshProcess *proc = client->getChannel<SshProcess>("monitor");

// Monitor state changes
QObject::connect(proc, &SshChannel::stateChanged,
    [](SshChannel::ChannelState state) {
        switch(state) {
            case SshChannel::Openning:
                qDebug() << "Opening channel...";
                break;
            case SshChannel::Exec:
                qDebug() << "Executing...";
                break;
            case SshChannel::Ready:
                qDebug() << "Ready!";
                break;
            case SshChannel::Error:
                qDebug() << "Error occurred";
                break;
            default:
                break;
        }
    });

proc->runCommand("echo Hello");
```

## Best Practices

1. **Channel Reuse**: Channels can be reused for multiple operations to reduce overhead
2. **State Monitoring**: Connect to `stateChanged` signal to track operation progress
3. **Error Handling**: Always check for `Error` state
4. **Cleanup**: Call `close()` when done with a channel
5. **Naming**: Use descriptive names for channels to aid debugging

## Thread Safety

Channels are not thread-safe and should be used from the same thread as their parent `SshClient`. Use Qt's signal/slot mechanism for cross-thread communication.

## See Also

- [SshClient](sshclient.md) - Main client class
- [SshProcess](sshprocess.md) - Process execution channel
- [SshSFtp](sshsftp.md) - SFTP channel
- [SSH Tunneling](tunneling.md) - Tunneling channels
