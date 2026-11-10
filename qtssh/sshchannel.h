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
#include <QMutex>
#include <libssh2.h>

class SshClient;

Q_DECLARE_LOGGING_CATEGORY(sshchannel)

class SshChannel : public QObject
{
    Q_OBJECT
    friend class SshClient;

public:
    QString name() const;
    virtual void close() = 0;

    enum ChannelState {
        Openning,
        Exec,
        Ready,
        Close,
        WaitClose,
        Freeing,
        Free,
        Error
    };
    Q_ENUM(ChannelState)

    ChannelState channelState() const;
    void setChannelState(const ChannelState &channelState);
    bool waitForState(SshChannel::ChannelState state);

    SshClient *sshClient() const;

protected:
    explicit SshChannel(QString name, SshClient *client);
    virtual ~SshChannel();
    SshClient *m_sshClient  {nullptr};
    QString m_name;

    /* Called by SshClient just before the libssh2 session is freed: every
     * libssh2 handle owned by the channel becomes invalid. Implementations
     * must forget their handles, then call SshChannel::sessionClosed(). */
    virtual void sessionClosed();

protected slots:
    virtual void sshDataReceived() {}

private:
    ChannelState m_channelState {ChannelState::Openning};

signals:
    void stateChanged(ChannelState state);
};
