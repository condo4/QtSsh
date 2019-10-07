#pragma once

#include "sshchannel.h"

class SshScpSend : public SshChannel
{
    Q_OBJECT

protected:
    SshScpSend(const QString &name, SshClient * client);
    friend class SshClient;

    void free();

public:
    virtual ~SshScpSend() override;
    QString send(const QString &source, QString dest);
    void close() override;

private:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
};
