#include "sshsftp.h"
#include "sshclient.h"

#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

Q_LOGGING_CATEGORY(logsshsftp, "ssh.sftp", QtWarningMsg)

SshSFtp::SshSFtp(const QString &name, SshClient *client):
    SshChannel(name, client)

{
    m_sftpSession = qssh2_sftp_init(m_sshClient->session());
    QObject::connect(client, &SshClient::sshDataReceived, this, &SshSFtp::sshDataReceived, Qt::QueuedConnection);

    qCDebug(logsshsftp) << "SFTP connected";
}

SshSFtp::~SshSFtp()
{
    qCDebug(logsshsftp) << "SshSFtp::~SshSFtp()";
    close();
    qCDebug(logsshsftp) << "SshSFtp::~SshSFtp() OK";
}

void SshSFtp::close()
{
    if(m_sftpSession)
    {
        qssh2_sftp_shutdown(m_sftpSession);
        m_sftpSession = nullptr;
        qCDebug(sshchannel) << "closeChannel:" << m_name;
    }
}

void SshSFtp::free()
{
    close();
}


QString SshSFtp::send(const QString &source, QString dest)
{
    QFile s(source);
    QFileInfo src(source);
    QByteArray buffer(1024 * 100, 0);
    size_t nread;
    char *ptr;
    long total = 0;
    FILE *local;
    ssize_t rc;
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
        qCWarning(logsshsftp) << "Can't open file "<< source;
        return "";
    }

    sftpfile = qssh2_sftp_open_ex(m_sshClient->session(), m_sftpSession, qPrintable(dest), static_cast<unsigned int>(dest.size()), LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
                     LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR| LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH, LIBSSH2_SFTP_OPENFILE);

    if(!sftpfile)
    {
        qCWarning(logsshsftp) << "SSHFS error";
        fclose(local);
        return "";
    }

    do {
         nread = fread(buffer.data(), 1, static_cast<unsigned int>(buffer.size()), local);
         if (nread <= 0) {
             /* end of file */
             break;
         }
         ptr = buffer.data();

         total += nread;

         do {
             rc = qssh2_sftp_write(sftpfile, ptr, nread);
             if(rc < 0)
             {
                 qCWarning(logsshsftp) << "Write error send(" << source << "," <<  dest << ") = " << rc;
                 break;
             }
             ptr += rc;
             nread -= static_cast<unsigned long>(rc);

         } while (nread);
     } while (rc > 0);

    fclose(local);
    qssh2_sftp_close_handle(sftpfile);

    return dest;
}

bool SshSFtp::get(const QString &source, QString dest, bool override)
{
    QFileInfo src(source);
    LIBSSH2_SFTP_HANDLE *sftpfile;
    QByteArray buffer(1024 * 100, 0);
    FILE *tempstorage;
    ssize_t rc = 0;

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
        qCWarning(logsshsftp) << "Can't open file "<< dest;
        return false;
    }

    sftpfile = qssh2_sftp_open_ex(m_sshClient->session(), m_sftpSession, qPrintable(source), static_cast<unsigned int>(source.size()), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
    if(!sftpfile)
    {
        qCWarning(logsshsftp) << "ERROR : SSH error " << rc;
        fclose(tempstorage);
        return false;
    }

    do {
        /* read in a loop until we block */
        rc = qssh2_sftp_read(sftpfile, buffer.data(), static_cast<unsigned int>(buffer.size()));

        if(rc > 0) {
            /* write to temporary storage area */
            fwrite(buffer.data(), static_cast<unsigned long>(rc), 1, tempstorage);
        }
    } while (rc > 0);

    qssh2_sftp_close_handle(sftpfile);

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

int SshSFtp::mkdir(const QString &dest)
{
    int res = qssh2_sftp_mkdir_ex(m_sftpSession, qPrintable(dest), static_cast<unsigned int>(dest.size()), 0775);
    if(res != 0)
    {
        qCWarning(logsshsftp) << "mkdir " << dest << " error, result = " << res;
    }
    else
    {
        qCDebug(logsshsftp) << "mkdir "<< dest << " OK";
    }

    return res;
}

QStringList SshSFtp::readdir(const QString &d)
{
    qCDebug(logsshsftp) << "readdir(" << d << ")";
    int rc;
    QStringList result;
    LIBSSH2_SFTP_HANDLE *sftpdir = qssh2_sftp_open_ex(m_sshClient->session(), m_sftpSession, qPrintable(d), static_cast<unsigned int>(d.size()), 0, 0, LIBSSH2_SFTP_OPENDIR);
    QByteArray buffer(512,0);

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    do {
        rc = qssh2_sftp_readdir_ex(sftpdir, buffer.data(), static_cast<unsigned int>(buffer.size()), nullptr, 0, &attrs);
        if(rc > 0) {
            result.append(QString(buffer));
        }
    } while(rc > 0);
    qssh2_sftp_close_handle(sftpdir);

    qCDebug(logsshsftp) << "readdir(" << d << ") " << result;
    return result;
}

bool SshSFtp::isDir(const QString &d)
{
    LIBSSH2_SFTP_ATTRIBUTES fileinfo = getFileInfo(d);
    qCDebug(logsshsftp) << "isDir(" << d << ") = " << LIBSSH2_SFTP_S_ISDIR(fileinfo.permissions);
    return LIBSSH2_SFTP_S_ISDIR(fileinfo.permissions);
}

bool SshSFtp::isFile(const QString &d)
{
    LIBSSH2_SFTP_ATTRIBUTES fileinfo = getFileInfo(d);
    qCDebug(logsshsftp) << "isFile(" << d << ") = " << LIBSSH2_SFTP_S_ISREG(fileinfo.permissions);
    return LIBSSH2_SFTP_S_ISREG(fileinfo.permissions);
}

int SshSFtp::mkpath(const QString &dest)
{
    qCDebug(logsshsftp) << "mkpath " << dest;
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

bool SshSFtp::unlink(const QString &d)
{
    ssize_t res = qssh2_sftp_unlink_ex(m_sftpSession, qPrintable(d), static_cast<unsigned int>(d.size()));
    if(res != 0)
    {
        qCWarning(logsshsftp) << "unlink " << d << " error, result = " << res;
    }
    else
    {
        qCDebug(logsshsftp) << "unlink "<< d << " OK";
    }
    return res;
}

quint64 SshSFtp::filesize(const QString &d)
{
    return getFileInfo(d).filesize;
}

void SshSFtp::sshDataReceived()
{
    // Nothing to do
}

LIBSSH2_SFTP_ATTRIBUTES SshSFtp::getFileInfo(const QString &path)
{
    if(!m_fileinfo.contains(path))
    {
        LIBSSH2_SFTP_ATTRIBUTES fileinfo;
        qssh2_sftp_stat_ex(m_sftpSession, qPrintable(path), static_cast<unsigned int>(path.size()), LIBSSH2_SFTP_STAT, &fileinfo);
        m_fileinfo[path] = fileinfo;
    }
    return m_fileinfo[path];
}
