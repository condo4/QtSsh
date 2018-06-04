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

class SshClient;

Q_DECLARE_LOGGING_CATEGORY(sshchannel)

class SshChannel : public QObject
{
    Q_OBJECT

protected:
    LIBSSH2_CHANNEL *sshChannel;
    SshClient       *sshClient;

public:
    explicit SshChannel(QObject *client);
    explicit SshChannel(SshClient *client);
    virtual ~SshChannel();

protected slots:
    virtual void sshDataReceived() {}
    virtual void stopChannel();

public slots:
    virtual quint16 localPort() { return 0; }
};
