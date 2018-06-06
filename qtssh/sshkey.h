#ifndef SSHKEY_H
#define SSHKEY_H

#include <QObject>

class SshKey : public QObject
{
    Q_OBJECT

public:
    explicit SshKey(QObject *parent = nullptr);
    virtual ~SshKey();

    enum Type {
        UnknownType,
        Rsa,
        Dss
    };
    Q_ENUM(Type)
    QByteArray hash;
    QByteArray key;
    Type       type;

signals:

public slots:
};

#endif // SSHKEY_H
