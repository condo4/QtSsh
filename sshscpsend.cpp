#include "sshscpsend.h"
#include "sshclient.h"
#include <QFileInfo>
#include <qdebug.h>

bool SshScpSend::state() const
{
    return _state;
}

void SshScpSend::sshDataReceived()
{
    static char mem[1024];
    static size_t nread;
    static char *ptr;
    int rc;

    switch(_currentState)
    {
    case ScpPrepare:
        if(!sshChannel) {
            sshChannel = libssh2_scp_send(sshClient->session(), qPrintable(_destination), _fileinfo.st_mode & 0777, (unsigned long)_fileinfo.st_size);

            if ((!sshChannel) && (libssh2_session_last_errno(sshClient->session()) != LIBSSH2_ERROR_EAGAIN)) {
                char *errmsg;
                int errlen;
                libssh2_session_last_error(sshClient->session(), &errmsg, &errlen, 0);
                qDebug() << "ERROR : Unable to open a session: " << errmsg;
            }
            if(sshChannel) {
#ifdef DEBUG_SCPSEND
                qDebug() << "DEBUG : ScpPrepare Send Accepted";
#endif
                _currentState = ScpCopy;
            }
        }
        break;

    case ScpCopy:
        do {
            nread = fread(mem, 1, sizeof(mem), _local);
            if (nread <= 0) {
                /* end of file */
                _currentState = ScpEof;
                sshDataReceived();
                break;
            }
            ptr = mem;
            do {
                /* write the same data over and over, until error or completion */
                rc = libssh2_channel_write(sshChannel, ptr, nread);

                if (rc < 0) {
                    qDebug() << "ERROR : Copy error " << rc;
                    _currentState = ScpError;
                    break;
                }
                else {
                    /* rc indicates how many bytes were written this time */
                    ptr += rc;
                    nread -= rc;
                }
            } while (nread);
        } while(1);
        break;
    case ScpEof:
#ifdef DEBUG_SCPSEND
        qDebug() << "DEBUG : Sending EOF";
#endif
        libssh2_channel_send_eof(sshChannel);
        _currentState = ScpClose;
        break;

    case ScpClose:
#ifdef DEBUG_SCPSEND
        qDebug() << "DEBUG : Waiting EOF";
#endif
        libssh2_channel_wait_eof(sshChannel);
        _state = true;
        _currentState = ScpEnd;
        stopChannel();
        emit transfertTerminate();
        break;

    case ScpError:
        qDebug() << "ERROR : SCP ERROR";
        _state = false;
        stopChannel();
        emit transfertTerminate();
        break;

    case ScpEnd:
#ifdef DEBUG_SCPSEND
        qDebug() << "DEBUG : Wait end";
#endif
        break;
    }
}

SshScpSend::SshScpSend(SshClient *client, QString source, QString dest):
    SshChannel(client),
    _source(source),
    _destination(dest)
{
    QObject::connect(client, &SshClient::sshDataReceived, this, &SshScpSend::sshDataReceived);
}

QString SshScpSend::send()
{
    QFileInfo src(_source);

    const char *loclfile=qPrintable(src.absoluteFilePath());

    _currentState = ScpPrepare;

    _local = fopen(loclfile, "rb");
    if (!_local) {
        qDebug() << "ERROR : Can't open local file " << loclfile;
        return "";
    }

    stat(loclfile, &_fileinfo);

    /* Send a file via scp. The mode parameter must only have permissions! */
#ifdef DEBUG_SCPSEND
    qDebug() << "DEBUG : Send to " << qPrintable(_destination + "/" + src.fileName());
#endif
    sshChannel = libssh2_scp_send(sshClient->session(), qPrintable(_destination + src.fileName()), _fileinfo.st_mode & 0777, (unsigned long)_fileinfo.st_size);

    if ((!sshChannel) && (libssh2_session_last_errno(sshClient->session()) != LIBSSH2_ERROR_EAGAIN)) {
        char *errmsg;
        int errlen;
        libssh2_session_last_error(sshClient->session(), &errmsg, &errlen, 0);
        qDebug() << "ERROR : Unable to open a session: " << errmsg;
        _currentState = ScpError;
    }
#ifdef DEBUG_SCPSEND
    qDebug() << "DEBUG : End of send function";
#endif
    return _destination + src.fileName();
}

