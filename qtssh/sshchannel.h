/**
 * \file sshchannel.h
 * \brief Generic Channel SSH class
 * \author Fabien Proriol
 * \version 1.0
 * \date 2018/03/22
 * \details This class is a Generic class for all Ssh Channel
 */

#pragma once

#include <QObject>
#include <QLoggingCategory>
#include "async_libssh2.h"
#include <QMutex>

class SshClient;
/* All SshChannel types */
class SshSFtp;
class SshTunnelIn;
class SshTunnelOut;
class SshTunnelOutSrv;

Q_DECLARE_LOGGING_CATEGORY(sshchannel)

class SshChannel : public QObject
{
    Q_OBJECT
    bool m_connected;

protected:
    explicit SshChannel(QString name, SshClient *client);

public:
    virtual ~SshChannel();
    bool connected() const;
    QString name() const;

protected:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
    SshClient       *m_sshClient  {nullptr};
    QString m_name;
    virtual void connectChannel(LIBSSH2_CHANNEL *channel);
    virtual void close();
    virtual void free();

protected slots:
    virtual void sshDataReceived() {}

public slots:
    virtual void disconnectChannel();
    virtual quint16 localPort() { return 0; }

    friend class SshClient;
};
