#ifndef SSHACCESS_H
#define SSHACCESS_H

class SshAccessInterface
{
public:
    virtual ~SshAccessInterface() {}
    virtual void disconnectFromHost() = 0;

};

#endif // SSHACCESS_H
