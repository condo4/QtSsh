#include "sshkey.h"

SshKey::SshKey(QObject *parent)
    : QObject(parent)
    , type(UnknownType)
{

}

SshKey::~SshKey()
{

}
