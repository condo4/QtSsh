#pragma once

#include "sshchannel.h"

class SshScpGet : public SshChannel
{
    Q_OBJECT

protected:
    SshScpGet(const QString &name, SshClient *client);
    friend class SshClient;

    void free();

public:
    virtual ~SshScpGet() override;
    QString get(const QString &source, const QString &dest);
    void close() override;

private:
    LIBSSH2_CHANNEL *m_sshChannel {nullptr};
};
