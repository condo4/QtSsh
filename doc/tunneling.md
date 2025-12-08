# SSH Tunneling API Documentation

## Overview

QtSsh provides two types of SSH tunneling (port forwarding):

- **SshTunnelOut** (Local/Direct Forwarding): Forward local port to remote destination
- **SshTunnelIn** (Remote/Reverse Forwarding): Forward remote port to local destination

## Class Hierarchy

```
QObject
  └── SshChannel
        ├── SshTunnelOut  (Local/Direct Forwarding)
        └── SshTunnelIn   (Remote/Reverse Forwarding)
```

## Headers

```cpp
#include "sshtunnelout.h"
#include "sshtunnelin.h"
```

## Creation

Tunnels are created through `SshClient::getChannel<T>()`:

```cpp
SshTunnelOut *tunnelOut = client->getChannel<SshTunnelOut>("localForward");
SshTunnelIn *tunnelIn = client->getChannel<SshTunnelIn>("remoteForward");
```

---

## SshTunnelOut (Local/Direct Forwarding)

Local port forwarding forwards connections from a local port through the SSH server to a destination.

**Use Case**: Access a service on a remote network through an SSH server.

```
[Local App] -> [Local Port] -> [SSH Server] -> [Remote Service]
```

### Methods

#### listen

```cpp
void listen(quint16 port, QString hostTarget = "127.0.0.1", QString hostListen = "127.0.0.1")
```

Starts listening on a local port and forwards connections.

**Parameters:**
- `port`: Local port to listen on (0 = auto-assign)
- `hostTarget`: Target hostname/IP on remote side (default: "127.0.0.1")
- `hostListen`: Local interface to listen on (default: "127.0.0.1")

**Example:**
```cpp
SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("forward");
tunnel->listen(8080, "localhost", "127.0.0.1");
// Connections to local port 8080 will be forwarded to localhost:8080 on remote side
```

#### localPort

```cpp
quint16 localPort()
```

Returns the actual local port being used.

**Returns:** Local port number

#### port

```cpp
quint16 port() const
```

Returns the configured port.

**Returns:** Port number

#### connections

```cpp
int connections()
```

Returns the number of active connections.

**Returns:** Connection count

#### closeAllConnections

```cpp
void closeAllConnections()
```

Closes all active tunnel connections.

#### close

```cpp
void close() override
```

Closes the tunnel.

### Signals

#### connectionChanged

```cpp
void connectionChanged(int count)
```

Emitted when the number of active connections changes.

**Parameters:**
- `count`: New connection count

---

## SshTunnelIn (Remote/Reverse Forwarding)

Remote port forwarding forwards connections from a remote port on the SSH server to a local destination.

**Use Case**: Expose a local service to the remote network through the SSH server.

```
[Remote App] -> [Remote Port on SSH Server] -> [SSH Tunnel] -> [Local Service]
```

### Methods

#### listen

```cpp
void listen(QString host, quint16 localPort, quint16 remotePort, QString listenHost = "127.0.0.1", int queueSize = 16)
```

Starts listening on a remote port and forwards connections to local destination.

**Parameters:**
- `host`: Target hostname/IP on local side
- `localPort`: Target port on local side
- `remotePort`: Port to listen on remote side
- `listenHost`: Interface to listen on remote side (default: "127.0.0.1")
- `queueSize`: Connection queue size (default: 16)

**Example:**
```cpp
SshTunnelIn *tunnel = client->getChannel<SshTunnelIn>("reverse");
tunnel->listen("localhost", 3000, 8080);
// Connections to port 8080 on SSH server will be forwarded to localhost:3000 locally
```

#### localPort

```cpp
quint16 localPort()
```

Returns the local target port.

**Returns:** Local port number

#### remotePort

```cpp
quint16 remotePort()
```

Returns the remote listening port.

**Returns:** Remote port number

#### close

```cpp
void close() override
```

Closes the tunnel.

### Signals

#### connectionChanged

```cpp
void connectionChanged(int count)
```

Emitted when the number of active connections changes.

---

## Usage Examples

### Local Port Forwarding (SshTunnelOut)

#### Access Remote MySQL Database

```cpp
// Forward local port 3306 to remote MySQL server
SshClient *client = new SshClient("mysql-tunnel");
client->setPassphrase("password");

connect(client, &SshClient::sshReady, [=]() {
    SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("mysql");

    connect(tunnel, &SshTunnelOut::connectionChanged, [](int count) {
        qDebug() << "Active connections:" << count;
    });

    // Forward local port 3306 to MySQL server at 192.168.1.100:3306
    tunnel->listen(3306, "192.168.1.100");

    qDebug() << "Tunnel established. Connect to MySQL at localhost:3306";
});

client->connectToHost("user", "ssh-server.com");
```

#### Access Internal Web Service

```cpp
SshClient *client = new SshClient("web-tunnel");
client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");

connect(client, &SshClient::sshReady, [=]() {
    SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("web");

    // Forward local port 8080 to internal web server
    tunnel->listen(8080, "internal-web.local");

    qDebug() << "Access internal web at http://localhost:8080";
});

client->connectToHost("user", "gateway.company.com");
```

#### Dynamic Port Assignment

```cpp
SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("dynamic");

// Use port 0 for automatic assignment
tunnel->listen(0, "remote-service.local");

// Get the assigned port
quint16 assignedPort = tunnel->localPort();
qDebug() << "Tunnel available at localhost:" << assignedPort;
```

#### Multiple Services Through One SSH Connection

```cpp
connect(client, &SshClient::sshReady, [=]() {
    // Database tunnel
    SshTunnelOut *dbTunnel = client->getChannel<SshTunnelOut>("db");
    dbTunnel->listen(5432, "db.internal");

    // Web service tunnel
    SshTunnelOut *webTunnel = client->getChannel<SshTunnelOut>("web");
    webTunnel->listen(8080, "web.internal");

    // API tunnel
    SshTunnelOut *apiTunnel = client->getChannel<SshTunnelOut>("api");
    apiTunnel->listen(3000, "api.internal");

    qDebug() << "All tunnels established:";
    qDebug() << "  Database: localhost:5432";
    qDebug() << "  Web:      localhost:8080";
    qDebug() << "  API:      localhost:3000";
});
```

### Remote Port Forwarding (SshTunnelIn)

#### Expose Local Web Server

```cpp
// Make local web server accessible from remote network
SshClient *client = new SshClient("expose-web");
client->setPassphrase("password");

connect(client, &SshClient::sshReady, [=]() {
    SshTunnelIn *tunnel = client->getChannel<SshTunnelIn>("expose");

    connect(tunnel, &SshTunnelIn::connectionChanged, [](int count) {
        qDebug() << "Remote connections:" << count;
    });

    // Forward remote port 8080 to local port 80
    tunnel->listen("localhost", 80, 8080);

    qDebug() << "Local web server exposed on SSH server port 8080";
});

client->connectToHost("user", "public-server.com");
```

#### Expose Local Development Server

```cpp
SshClient *client = new SshClient("dev-expose");
client->setKeys("~/.ssh/id_rsa.pub", "~/.ssh/id_rsa");

connect(client, &SshClient::sshReady, [=]() {
    SshTunnelIn *tunnel = client->getChannel<SshTunnelIn>("dev");

    // Forward remote port 3000 to local development server
    tunnel->listen("localhost", 3000, 3000);

    qDebug() << "Development server accessible at public-server.com:3000";
});

client->connectToHost("user", "public-server.com");
```

#### Temporary File Sharing

```cpp
// Start simple HTTP server locally (Python example: python3 -m http.server 8000)
// Then expose it temporarily

SshClient *client = new SshClient("file-share");
client->setPassphrase("password");

connect(client, &SshClient::sshReady, [=]() {
    SshTunnelIn *tunnel = client->getChannel<SshTunnelIn>("share");

    // Expose local HTTP server on remote port 8000
    tunnel->listen("localhost", 8000, 8000);

    qDebug() << "Files accessible at http://server.com:8000";
    qDebug() << "Press Ctrl+C to stop sharing";
});

client->connectToHost("user", "server.com");
```

### Advanced Examples

#### Connection Monitoring

```cpp
class TunnelMonitor : public QObject {
    Q_OBJECT
public:
    TunnelMonitor(SshTunnelOut *tunnel) : m_tunnel(tunnel) {
        connect(tunnel, &SshTunnelOut::connectionChanged,
                this, &TunnelMonitor::onConnectionChanged);
    }

private slots:
    void onConnectionChanged(int count) {
        if (count > m_maxConnections) {
            m_maxConnections = count;
        }
        m_totalChanges++;

        qDebug() << QString("Connections: %1 (Peak: %2, Changes: %3)")
                    .arg(count)
                    .arg(m_maxConnections)
                    .arg(m_totalChanges);
    }

private:
    SshTunnelOut *m_tunnel;
    int m_maxConnections = 0;
    int m_totalChanges = 0;
};

// Usage
SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>("monitored");
TunnelMonitor *monitor = new TunnelMonitor(tunnel);
tunnel->listen(8080, "remote-service");
```

#### Auto-Reconnecting Tunnel

```cpp
class AutoTunnel : public QObject {
    Q_OBJECT
public:
    AutoTunnel(const QString &user, const QString &host, quint16 localPort, const QString &remoteHost)
        : m_user(user), m_host(host), m_localPort(localPort), m_remoteHost(remoteHost) {
        m_client = new SshClient("auto-tunnel", this);
        connect(m_client, &SshClient::sshReady, this, &AutoTunnel::onConnected);
        connect(m_client, &SshClient::sshDisconnected, this, &AutoTunnel::onDisconnected);
    }

    void start(const QString &password) {
        m_client->setPassphrase(password);
        m_client->connectToHost(m_user, m_host);
    }

private slots:
    void onConnected() {
        qDebug() << "Tunnel connected";
        SshTunnelOut *tunnel = m_client->getChannel<SshTunnelOut>("main");
        tunnel->listen(m_localPort, m_remoteHost);
    }

    void onDisconnected() {
        qDebug() << "Tunnel disconnected, reconnecting in 5 seconds...";
        QTimer::singleShot(5000, [=]() {
            m_client->connectToHost(m_user, m_host);
        });
    }

private:
    SshClient *m_client;
    QString m_user;
    QString m_host;
    quint16 m_localPort;
    QString m_remoteHost;
};
```

#### Tunnel Manager

```cpp
class TunnelManager {
public:
    void addTunnel(const QString &name, quint16 localPort, const QString &remoteHost, quint16 remotePort = 0) {
        TunnelConfig config{name, localPort, remoteHost, remotePort};
        m_configs.append(config);
    }

    void startAll(SshClient *client) {
        for (const auto &config : m_configs) {
            SshTunnelOut *tunnel = client->getChannel<SshTunnelOut>(config.name);
            tunnel->listen(config.localPort, config.remoteHost);

            qDebug() << QString("Tunnel '%1': localhost:%2 -> %3:%4")
                        .arg(config.name)
                        .arg(config.localPort)
                        .arg(config.remoteHost)
                        .arg(config.remotePort ? config.remotePort : config.localPort);
        }
    }

private:
    struct TunnelConfig {
        QString name;
        quint16 localPort;
        QString remoteHost;
        quint16 remotePort;
    };

    QList<TunnelConfig> m_configs;
};

// Usage
TunnelManager manager;
manager.addTunnel("mysql", 3306, "db.internal");
manager.addTunnel("web", 8080, "web.internal");
manager.addTunnel("redis", 6379, "cache.internal");

connect(client, &SshClient::sshReady, [&]() {
    manager.startAll(client);
});
```

#### SOCKS Proxy Alternative

While QtSsh doesn't provide native SOCKS proxy, you can create multiple tunnels:

```cpp
void createDevelopmentTunnels(SshClient *client) {
    // Database
    auto db = client->getChannel<SshTunnelOut>("postgres");
    db->listen(5432, "postgres.internal");

    // Cache
    auto cache = client->getChannel<SshTunnelOut>("redis");
    cache->listen(6379, "redis.internal");

    // Message Queue
    auto mq = client->getChannel<SshTunnelOut>("rabbitmq");
    mq->listen(5672, "rabbitmq.internal");

    // Web Admin
    auto admin = client->getChannel<SshTunnelOut>("admin");
    admin->listen(8080, "admin.internal");

    qDebug() << "Development environment tunneled";
}
```

## Common Use Cases

### Database Access

```cpp
// PostgreSQL
tunnel->listen(5432, "postgres-server.internal");

// MySQL
tunnel->listen(3306, "mysql-server.internal");

// MongoDB
tunnel->listen(27017, "mongo-server.internal");

// Redis
tunnel->listen(6379, "redis-server.internal");
```

### Web Services

```cpp
// Internal web application
tunnel->listen(8080, "internal-web");

// API endpoint
tunnel->listen(3000, "api.internal");

// Elasticsearch
tunnel->listen(9200, "elasticsearch.internal");
```

### Remote Desktop

```cpp
// VNC
tunnel->listen(5900, "desktop-machine");

// RDP
tunnel->listen(3389, "windows-machine");
```

## Best Practices

1. **Security**: Only bind to 127.0.0.1 unless you need to expose the tunnel to your local network
2. **Port Selection**: Use non-privileged ports (>1024) to avoid requiring root
3. **Connection Monitoring**: Monitor `connectionChanged` for debugging
4. **Resource Cleanup**: Close tunnels when no longer needed
5. **Error Handling**: Handle SSH disconnections gracefully
6. **Firewall**: Ensure remote firewalls allow the target connections

## Tunnel Types Comparison

| Feature | SshTunnelOut (Local) | SshTunnelIn (Remote) |
|---------|---------------------|---------------------|
| **Direction** | Local → SSH → Remote | Remote → SSH → Local |
| **Listens On** | Local machine | SSH server |
| **Use Case** | Access remote services | Expose local services |
| **Example** | Access internal DB | Share local dev server |
| **Server Config** | No special config | May need AllowTcpForwarding |

## Troubleshooting

### Tunnel Not Working

1. **Check SSH server configuration**:
   - Local forwarding: Usually enabled by default
   - Remote forwarding: Requires `AllowTcpForwarding yes` in sshd_config

2. **Check firewall rules** on target machine

3. **Verify port availability**:
```cpp
quint16 port = tunnel->localPort();
qDebug() << "Tunnel using port:" << port;
```

4. **Monitor connections**:
```cpp
connect(tunnel, &SshTunnelOut::connectionChanged, [](int count) {
    qDebug() << "Active connections:" << count;
});
```

### Permission Denied

Remote forwarding may require server configuration:
```bash
# In /etc/ssh/sshd_config
AllowTcpForwarding yes
GatewayPorts yes  # If you need to bind to 0.0.0.0
```

## See Also

- [SshClient](sshclient.md) - Main client class
- [SshChannel](sshchannel.md) - Base channel class
- [Quick Start Guide](quickstart.md) - Getting started examples
