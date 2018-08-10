#include "sshsftp.h"
#include "sshclient.h"

#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

QString SshSFtp::send(QString source, QString dest)
{
    QFile s(source);
    QFileInfo src(source);
    QByteArray buffer(1024 * 100, 0);
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
        sftpfile = libssh2_sftp_open(m_sftpSession, qPrintable(dest),
                                 LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
                                     LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
                                                               LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
        rc = libssh2_session_last_errno(sshClient->session());
        if (!sftpfile && (rc == LIBSSH2_ERROR_EAGAIN))
        {
            m_waitData(2000);
        }
        else
        {
            if(!sftpfile)
            {
                qDebug() << "ERROR : SSH error " << rc;
                fclose(local);
                return "";
            }
        }
    } while (!sftpfile);

    do {
         nread = fread(buffer.data(), 1, buffer.size(), local);
         if (nread <= 0) {
             /* end of file */
             break;
         }
         ptr = buffer.data();

         total += nread;

         do {
             while ((rc = libssh2_sftp_write(sftpfile, ptr, nread)) == LIBSSH2_ERROR_EAGAIN)
             {
                 m_waitData(2000);
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
    QByteArray buffer(1024 * 100, 0);
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
        sftpfile = libssh2_sftp_open(m_sftpSession, qPrintable(source), LIBSSH2_FXF_READ, 0);
        rc = libssh2_session_last_errno(sshClient->session());
        if (!sftpfile && (rc == LIBSSH2_ERROR_EAGAIN))
        {
            m_waitData(2000);
        }
        else
        {
            if(!sftpfile)
            {
                qDebug() << "ERROR : SSH error " << rc;
                fclose(tempstorage);
                return false;
            }
        }
    } while (!sftpfile);

    emit xfer();
    do {
        do {
            /* read in a loop until we block */
            xfer();
            rc = libssh2_sftp_read(sftpfile, buffer.data(), buffer.size());

            if(rc > 0) {
                /* write to temporary storage area */
                fwrite(buffer.data(), rc, 1, tempstorage);
            }
        } while (rc > 0);

        if(rc != LIBSSH2_ERROR_EAGAIN) {
            /* error or end of file */
            break;
        }

        m_waitData(1000);
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

    while((res = libssh2_sftp_mkdir(m_sftpSession, qPrintable(dest), 0775)) == LIBSSH2_ERROR_EAGAIN)
    {
        m_waitData(2000);
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
    QByteArray buffer(512,0);

    do {
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        /* loop until we fail */
        while ((rc = libssh2_sftp_readdir(sftpdir, buffer.data(), buffer.size(), &attrs)) == LIBSSH2_ERROR_EAGAIN)
        {
            m_waitData(2000);
        }
        if(rc > 0) {
            result.append(QString(buffer));
        }
        else if (rc != LIBSSH2_ERROR_EAGAIN) {
            break;
        }

    } while (1);
    closeDirHandler(qPrintable(d));
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
        status = libssh2_sftp_stat(m_sftpSession,qPrintable(d),&fileinfo);
        if(status == LIBSSH2_ERROR_EAGAIN) m_waitData(2000);
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

    while((res = libssh2_sftp_unlink(m_sftpSession, qPrintable(d))) == LIBSSH2_ERROR_EAGAIN)
    {
        m_waitData(2000);
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

bool SshSFtp::m_waitData(int timeout)
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
    if(!m_dirhandler.contains(path))
    {
        LIBSSH2_SFTP_HANDLE *sftpdir = NULL;
        do {
            sftpdir = libssh2_sftp_opendir(m_sftpSession, qPrintable(path));
            rc = libssh2_session_last_errno(sshClient->session());
            if (!sftpdir && (rc == LIBSSH2_ERROR_EAGAIN))
            {
                m_waitData(2000);
            }
            else if(!sftpdir && (rc == LIBSSH2_ERROR_SFTP_PROTOCOL))
            {
                    return NULL;
            }
        } while (!sftpdir);
        m_dirhandler[path] = sftpdir;
    }
    return m_dirhandler[path];
}

LIBSSH2_SFTP_HANDLE *SshSFtp::closeDirHandler(QString path)
{
    if(m_dirhandler.contains(path))
    {
        LIBSSH2_SFTP_HANDLE *sftpdir = m_dirhandler[path];
        libssh2_sftp_closedir(sftpdir);
        m_dirhandler.remove(path);
    }
    return NULL;
}

LIBSSH2_SFTP_ATTRIBUTES SshSFtp::getFileInfo(QString path)
{
    if(!m_fileinfo.contains(path))
    {
        LIBSSH2_SFTP_ATTRIBUTES fileinfo;
        int status = LIBSSH2_ERROR_EAGAIN;
        while(status == LIBSSH2_ERROR_EAGAIN)
        {
            status = libssh2_sftp_stat(m_sftpSession,qPrintable(path),&fileinfo);
            if(status == LIBSSH2_ERROR_EAGAIN) m_waitData(2000);
            else {
                m_fileinfo[path] = fileinfo;
            }
        }
    }
    return m_fileinfo[path];
}

SshSFtp::SshSFtp(SshClient *client):
    SshChannel(client)

{
    while(!(m_sftpSession = libssh2_sftp_init(sshClient->session())))
    {
        if(libssh2_session_last_errno(sshClient->session()) == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
#ifdef DEBUG_SFTP
            qDebug() << "DEBUG : SFTP try again";
#endif
        }
        else
        {
            break;
        }
    }
    if(!m_sftpSession)
    {
        qDebug() << "LAST ERROR IS : " << libssh2_session_last_errno(sshClient->session());
    }
    QObject::connect(client, &SshClient::sshDataReceived, this, &SshSFtp::sshDataReceived, Qt::QueuedConnection);

#ifdef DEBUG_SFTP
    qDebug() << "DEBUG : SFTP connected";
#endif
}

SshSFtp::~SshSFtp()
{
    foreach(LIBSSH2_SFTP_HANDLE *sftpdir, m_dirhandler.values())
    {
        libssh2_sftp_closedir(sftpdir);
    }
    libssh2_sftp_shutdown(m_sftpSession);
}

void SshSFtp::enableSFTP()
{

}
