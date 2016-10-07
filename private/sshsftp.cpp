#include "sshsftp.h"
#include "sshclient.h"

#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

QString SshSFtp::send(QString source, QString dest)
{
    QFile s(source);
    QFileInfo src(source);
    char mem[1024 * 100];
    size_t nread;
    char *ptr;
    long total = 0;
    FILE *local;
    int rc;
    LIBSSH2_SFTP_HANDLE *sftpfile;
    if(dest.endsWith("/"))
    {
        if(!isDir(dest))
        {
            mkpath(dest);
        }
        dest += src.fileName();
    }

    local = fopen(qPrintable(source), "rb");
    if (!local) {
        qDebug() << "ERROR : Can't open file "<< source;
        return "";
    }


    do {
        sftpfile = libssh2_sftp_open(_sftpSession, qPrintable(dest),
                                 LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
                                     LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
                                                               LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
        rc = libssh2_session_last_errno(sshClient->session());
        if (!sftpfile && (rc == LIBSSH2_ERROR_EAGAIN))
        {
            _waitData(2000);
        }
        else
        {
            if(!sftpfile)
            {
                qDebug() << "ERROR : SSH error " << rc;
                return "";
            }
        }
    } while (!sftpfile);

    do {
         nread = fread(mem, 1, sizeof(mem), local);
         if (nread <= 0) {
             /* end of file */
             break;
         }
         ptr = mem;

         total += nread;

         do {
             while ((rc = libssh2_sftp_write(sftpfile, ptr, nread)) == LIBSSH2_ERROR_EAGAIN)
             {
                 _waitData(2000);
             }
             if(rc < 0)
             {
                 qDebug() << "ERROR : Write error send(" << source << "," <<  dest << ") = " << rc;
                 break;
             }
             ptr += rc;
             nread -= rc;

         } while (nread);
     } while (rc > 0);

    fclose(local);
    libssh2_sftp_close(sftpfile);

    return dest;
}

bool SshSFtp::get(QString source, QString dest, bool override)
{
    QFileInfo src(source);
    LIBSSH2_SFTP_HANDLE *sftpfile;
    char mem[1024 * 100];
    FILE *tempstorage;
    int rc;

    if(dest.endsWith("/"))
    {
        dest += src.fileName();
    }
    QString original(dest);

    if(!override)
    {
        QFile fout(dest);
        if(fout.exists())
        {
            QString newpath;
            int i = 1;
            do {
                newpath = QString("%1.%2").arg(dest).arg(i);
                fout.setFileName(newpath);
                ++i;
            } while(fout.exists());
            dest = newpath;
        }
    }

    tempstorage = fopen(qPrintable(dest), "w");
    if (!tempstorage) {
        qDebug() << "ERROR : Can't open file "<< dest;
        return false;
    }

    do {
        sftpfile = libssh2_sftp_open(_sftpSession, qPrintable(source), LIBSSH2_FXF_READ, 0);
        rc = libssh2_session_last_errno(sshClient->session());
        if (!sftpfile && (rc == LIBSSH2_ERROR_EAGAIN))
        {
            _waitData(2000);
        }
        else
        {
            if(!sftpfile)
            {
                qDebug() << "ERROR : SSH error " << rc;
                return "";
            }
        }
    } while (!sftpfile);

    emit xfer();
    do {
        do {
            /* read in a loop until we block */
            xfer();
            rc = libssh2_sftp_read(sftpfile, mem, sizeof(mem));

            if(rc > 0) {
                /* write to temporary storage area */
                fwrite(mem, rc, 1, tempstorage);
            }
        } while (rc > 0);

        if(rc != LIBSSH2_ERROR_EAGAIN) {
            /* error or end of file */
            break;
        }

        _waitData(1000);
    } while (1);

    libssh2_sftp_close(sftpfile);

    fclose(tempstorage);

    /* Remove file if is the same that original */
    if(dest != original)
    {
        QCryptographicHash hash1( QCryptographicHash::Md5 );
        QCryptographicHash hash2( QCryptographicHash::Md5 );
        QFile f1( original );
        if ( f1.open( QIODevice::ReadOnly ) ) {
            hash1.addData( f1.readAll() );
        }
        QByteArray sig1 = hash1.result();
        QFile f2( dest );
        if ( f2.open( QIODevice::ReadOnly ) ) {
            hash2.addData( f2.readAll() );
        }
        QByteArray sig2 = hash2.result();
        if(sig1 == sig2)
        {
            f2.remove();
        }
    }
    return true;
}

int SshSFtp::mkdir(QString dest)
{
    int res;

    while((res = libssh2_sftp_mkdir(_sftpSession, qPrintable(dest), 0775)) == LIBSSH2_ERROR_EAGAIN)
    {
        _waitData(2000);
    }

    if(res != 0)
    {
        qDebug() << "ERROR : mkdir " << dest << " error, result = " << res;
    }
#ifdef DEBUG_SFTP
    else
    {
        qDebug() << "DEBUG : mkdir "<< dest << " OK";
    }
#endif

    return res;
}

QStringList SshSFtp::readdir(QString d)
{
    int rc;
    QStringList result;
    LIBSSH2_SFTP_HANDLE *sftpdir = getDirHandler(qPrintable(d));

    do {
        char mem[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        /* loop until we fail */
        while ((rc = libssh2_sftp_readdir(sftpdir, mem, sizeof(mem), &attrs)) == LIBSSH2_ERROR_EAGAIN)
        {
            _waitData(2000);
        }
        if(rc > 0) {
            result.append(QString(mem));
        }
        else if (rc != LIBSSH2_ERROR_EAGAIN) {
            break;
        }

    } while (1);
    return result;
}

bool SshSFtp::isDir(QString d)
{
    LIBSSH2_SFTP_HANDLE *sftpdir = getDirHandler(qPrintable(d));
    return sftpdir != NULL;
}

bool SshSFtp::isFile(QString d)
{
    LIBSSH2_SFTP_ATTRIBUTES fileinfo;
    int status = LIBSSH2_ERROR_EAGAIN;
    while(status == LIBSSH2_ERROR_EAGAIN)
    {
        status = libssh2_sftp_stat(_sftpSession,qPrintable(d),&fileinfo);
        if(status == LIBSSH2_ERROR_EAGAIN) _waitData(2000);
    }
#ifdef DEBUG_SFTP
    qDebug() << "DEBUG : isFile(" << d << ") = " << status;
#endif
    return (status == 0);
}

int SshSFtp::mkpath(QString dest)
{
#ifdef DEBUG_SFTP
    qDebug() << "DEBUG : mkpath " << dest;
#endif
    if(isDir(dest)) return true;
    QStringList d = dest.split("/");
    d.pop_back();
    if(mkpath(d.join("/")))
    {
        mkdir(dest);
    }
    if(isDir(dest)) return true;
    return false;
}

bool SshSFtp::unlink(QString d)
{
    int res;

    while((res = libssh2_sftp_unlink(_sftpSession, qPrintable(d))) == LIBSSH2_ERROR_EAGAIN)
    {
        _waitData(2000);
    }

    if(res != 0)
    {
        qDebug() << "ERROR : unlink " << d << " error, result = " << res;
    }
#ifdef DEBUG_SFTP
    else
    {
        qDebug() << "DEBUG : unlink "<< d << " OK";
    }
#endif
    return res;
}

quint64 SshSFtp::filesize(QString d)
{
    return getFileInfo(d).filesize;
}

void SshSFtp::sshDataReceived()
{
    emit sshData();
}

bool SshSFtp::_waitData(int timeout)
{
    bool ret;
    QEventLoop datawait;
    QTimer timer;
    QObject::connect(this, SIGNAL(sshData()), &datawait, SLOT(quit()));
    QObject::connect(&timer, SIGNAL(timeout()), &datawait, SLOT(quit()));
    timer.setInterval(timeout);
    timer.start();
    datawait.exec();
    ret = timer.isActive();
    timer.stop();
    return ret;
}

LIBSSH2_SFTP_HANDLE *SshSFtp::getDirHandler(QString path)
{
    int rc;
    if(!_dirhandler.contains(path))
    {
        LIBSSH2_SFTP_HANDLE *sftpdir = NULL;
        do {
            sftpdir = libssh2_sftp_opendir(_sftpSession, qPrintable(path));
            rc = libssh2_session_last_errno(sshClient->session());
            if (!sftpdir && (rc == LIBSSH2_ERROR_EAGAIN))
            {
                _waitData(2000);
            }
            else if(!sftpdir && (rc == LIBSSH2_ERROR_SFTP_PROTOCOL))
            {
                    return NULL;
            }
        } while (!sftpdir);
        _dirhandler[path] = sftpdir;
    }
    return _dirhandler[path];
}

LIBSSH2_SFTP_ATTRIBUTES SshSFtp::getFileInfo(QString path)
{
    if(!_fileinfo.contains(path))
    {
        LIBSSH2_SFTP_ATTRIBUTES fileinfo;
        int status = LIBSSH2_ERROR_EAGAIN;
        while(status == LIBSSH2_ERROR_EAGAIN)
        {
            status = libssh2_sftp_stat(_sftpSession,qPrintable(path),&fileinfo);
            if(status == LIBSSH2_ERROR_EAGAIN) _waitData(2000);
            else {
                _fileinfo[path] = fileinfo;
            }
        }
    }
    return _fileinfo[path];
}

SshSFtp::SshSFtp(SshClient *client):
    SshChannel(client)

{
    QObject::connect(client, &SshClient::sshDataReceived, this, &SshSFtp::sshDataReceived);

    while(!(_sftpSession = libssh2_sftp_init(sshClient->session())))
    {
        _waitData(2000);
    }
#ifdef DEBUG_SFTP
    qDebug() << "DEBUG : SFTP connected";
#endif
}

SshSFtp::~SshSFtp()
{
    foreach(LIBSSH2_SFTP_HANDLE *sftpdir, _dirhandler.values())
    {
        libssh2_sftp_closedir(sftpdir);
    }
    libssh2_sftp_shutdown(_sftpSession);
}

void SshSFtp::enableSFTP()
{

}
