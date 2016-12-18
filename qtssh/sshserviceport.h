#ifndef SSHSERVICE_H
#define SSHSERVICE_H

#include <QtGlobal>

class SshServicePort
{
public:
    SshServicePort() {}
    virtual ~SshServicePort() {}

    virtual quint16 localPort() = 0;
};

#endif // SSHSERVICE_H
