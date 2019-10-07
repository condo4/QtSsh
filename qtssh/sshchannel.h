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

Q_DECLARE_LOGGING_CATEGORY(sshchannel)

class SshChannel : public QObject
{
    Q_OBJECT
    friend class SshClient;

public:
    QString name() const;
    virtual void close() = 0;

protected:
    explicit SshChannel(QString name, SshClient *client);
    SshClient *m_sshClient  {nullptr};
    QString m_name;

protected slots:
    virtual void sshDataReceived() {}

signals:
    void canBeDestroy(SshChannel *);
};
