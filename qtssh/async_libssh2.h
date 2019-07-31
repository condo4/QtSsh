#pragma once

extern "C" {
#include <libssh2.h>
#include <libssh2_sftp.h>
}

#include <QCoreApplication>

inline const char* sshErrorToString(int err)
{
    switch(err)
    {
    case LIBSSH2_ERROR_SOCKET_NONE:
        return "LIBSSH2_ERROR_SOCKET_NONE";
    case LIBSSH2_ERROR_BANNER_RECV:
        return "LIBSSH2_ERROR_BANNER_RECV";
    case LIBSSH2_ERROR_BANNER_SEND:
        return "LIBSSH2_ERROR_BANNER_SEND";
    case LIBSSH2_ERROR_INVALID_MAC:
        return "LIBSSH2_ERROR_INVALID_MAC";
    case LIBSSH2_ERROR_KEX_FAILURE:
        return "LIBSSH2_ERROR_KEX_FAILURE";
    case LIBSSH2_ERROR_ALLOC:
        return "LIBSSH2_ERROR_ALLOC";
    case LIBSSH2_ERROR_SOCKET_SEND:
        return "LIBSSH2_ERROR_SOCKET_SEND";
    case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE:
        return "LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE";
    case LIBSSH2_ERROR_TIMEOUT:
        return "LIBSSH2_ERROR_TIMEOUT";
    case LIBSSH2_ERROR_HOSTKEY_INIT:
        return "LIBSSH2_ERROR_HOSTKEY_INIT";
    case LIBSSH2_ERROR_HOSTKEY_SIGN:
        return "LIBSSH2_ERROR_HOSTKEY_SIGN";
    case LIBSSH2_ERROR_DECRYPT:
        return "LIBSSH2_ERROR_DECRYPT";
    case LIBSSH2_ERROR_SOCKET_DISCONNECT:
        return "LIBSSH2_ERROR_SOCKET_DISCONNECT";
    case LIBSSH2_ERROR_PROTO:
        return "LIBSSH2_ERROR_PROTO";
    case LIBSSH2_ERROR_PASSWORD_EXPIRED:
        return "LIBSSH2_ERROR_PASSWORD_EXPIRED";
    case LIBSSH2_ERROR_FILE:
        return "LIBSSH2_ERROR_FILE";
    case LIBSSH2_ERROR_METHOD_NONE:
        return "LIBSSH2_ERROR_METHOD_NONE";
    case LIBSSH2_ERROR_AUTHENTICATION_FAILED:
        return "LIBSSH2_ERROR_AUTHENTICATION_FAILED";
    case LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED:
        return "LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED";
    case LIBSSH2_ERROR_CHANNEL_OUTOFORDER:
        return "LIBSSH2_ERROR_CHANNEL_OUTOFORDER";
    case LIBSSH2_ERROR_CHANNEL_FAILURE:
        return "LIBSSH2_ERROR_CHANNEL_FAILURE";
    case LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED:
        return "LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED";
    case LIBSSH2_ERROR_CHANNEL_UNKNOWN:
        return "LIBSSH2_ERROR_CHANNEL_UNKNOWN";
    case LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED:
        return "LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED";
    case LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED:
        return "LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED";
    case LIBSSH2_ERROR_CHANNEL_CLOSED:
        return "LIBSSH2_ERROR_CHANNEL_CLOSED";
    case LIBSSH2_ERROR_CHANNEL_EOF_SENT:
        return "LIBSSH2_ERROR_CHANNEL_EOF_SENT";
    case LIBSSH2_ERROR_SCP_PROTOCOL:
        return "LIBSSH2_ERROR_SCP_PROTOCOL";
    case LIBSSH2_ERROR_ZLIB:
        return "LIBSSH2_ERROR_ZLIB";
    case LIBSSH2_ERROR_SOCKET_TIMEOUT:
        return "LIBSSH2_ERROR_SOCKET_TIMEOUT";
    case LIBSSH2_ERROR_SFTP_PROTOCOL:
        return "LIBSSH2_ERROR_SFTP_PROTOCOL";
    case LIBSSH2_ERROR_REQUEST_DENIED:
        return "LIBSSH2_ERROR_REQUEST_DENIED";
    case LIBSSH2_ERROR_METHOD_NOT_SUPPORTED:
        return "LIBSSH2_ERROR_METHOD_NOT_SUPPORTED";
    case LIBSSH2_ERROR_INVAL:
        return "LIBSSH2_ERROR_INVAL";
    case LIBSSH2_ERROR_INVALID_POLL_TYPE:
        return "LIBSSH2_ERROR_INVALID_POLL_TYPE";
    case LIBSSH2_ERROR_PUBLICKEY_PROTOCOL:
        return "LIBSSH2_ERROR_PUBLICKEY_PROTOCOL";
    case LIBSSH2_ERROR_EAGAIN:
        return "LIBSSH2_ERROR_EAGAIN";
    case LIBSSH2_ERROR_BUFFER_TOO_SMALL:
        return "LIBSSH2_ERROR_BUFFER_TOO_SMALL";
    case LIBSSH2_ERROR_BAD_USE:
        return "LIBSSH2_ERROR_BAD_USE";
    case LIBSSH2_ERROR_COMPRESS:
        return "LIBSSH2_ERROR_COMPRESS";
    case LIBSSH2_ERROR_OUT_OF_BOUNDARY:
        return "LIBSSH2_ERROR_OUT_OF_BOUNDARY";
    case LIBSSH2_ERROR_AGENT_PROTOCOL:
        return "LIBSSH2_ERROR_AGENT_PROTOCOL";
    case LIBSSH2_ERROR_SOCKET_RECV:
        return "LIBSSH2_ERROR_SOCKET_RECV";
    case LIBSSH2_ERROR_ENCRYPT:
        return "LIBSSH2_ERROR_ENCRYPT";
    case LIBSSH2_ERROR_BAD_SOCKET:
        return "LIBSSH2_ERROR_BAD_SOCKET";
    case LIBSSH2_ERROR_KNOWN_HOSTS:
        return "LIBSSH2_ERROR_KNOWN_HOSTS";
    default:
        return "Unknown LIBSSH2 ERROR";
    }
}


/* Non-Blocking async functions */
inline int qssh2_session_handshake(LIBSSH2_SESSION *session, libssh2_socket_t sock)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_session_handshake(session, sock);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

/* Non-Blocking async functions */
inline char * qssh2_userauth_list(LIBSSH2_SESSION *session, const char *username, unsigned int username_len)
{
    char *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;
        cret = libssh2_userauth_list(session, username, username_len);
        ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
            continue;
        }
        break;
    }
    return cret;
}

inline int qssh2_userauth_publickey_frommemory(
        LIBSSH2_SESSION *session,
        const char *username,
        size_t username_len,
        const char *publickeyfiledata,
        size_t publickeyfiledata_len,
        const char *privatekeyfiledata,
        size_t privatekeyfiledata_len,
        const char *passphrase)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_userauth_publickey_frommemory(
                    session,
                    username,
                    username_len,
                    publickeyfiledata,
                    publickeyfiledata_len,
                    privatekeyfiledata,
                    privatekeyfiledata_len,
                    passphrase);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}


inline int qssh2_userauth_password_ex(LIBSSH2_SESSION *session,
                                             const char *username,
                                             unsigned int username_len,
                                             const char *password,
                                             unsigned int password_len)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_userauth_password_ex(session, username, username_len, password, password_len, nullptr);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_session_disconnect(LIBSSH2_SESSION *session, const char *description)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_session_disconnect_ex(session, SSH_DISCONNECT_BY_APPLICATION, description, "");
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_session_free(LIBSSH2_SESSION *session)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_session_free(session);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}


inline int qssh2_channel_close(LIBSSH2_CHANNEL *&channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_close(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_wait_closed(LIBSSH2_CHANNEL *&channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_wait_closed(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_free(LIBSSH2_CHANNEL *&channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_free(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    if(ret == 0)
    {
        channel = nullptr;
    }
    return ret;
}

inline ssize_t qssh2_channel_read(LIBSSH2_CHANNEL *&channel, char *buf, size_t buflen)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_read_ex(channel, 0, buf, buflen);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline LIBSSH2_CHANNEL * qssh2_channel_open(LIBSSH2_SESSION *session)
{
    LIBSSH2_CHANNEL *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_channel_open_ex(session, "session", sizeof("session") - 1, LIBSSH2_CHANNEL_WINDOW_DEFAULT, LIBSSH2_CHANNEL_PACKET_DEFAULT, nullptr, 0);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}

#include <qdebug.h>
inline LIBSSH2_CHANNEL * qssh2_scp_recv2(LIBSSH2_SESSION *session, const char *path, libssh2_struct_stat *sb)
{
    LIBSSH2_CHANNEL *cret = nullptr;

    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_scp_recv2(session, path, sb);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}


inline LIBSSH2_CHANNEL * qssh2_scp_send64(LIBSSH2_SESSION *session, const char *path, int mode, libssh2_int64_t size, time_t mtime, time_t atime)
{
    LIBSSH2_CHANNEL *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_scp_send64(session, path, mode, size, mtime, atime);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}

inline LIBSSH2_CHANNEL * qssh2_channel_forward_accept(LIBSSH2_SESSION *session, LIBSSH2_LISTENER *listener)
{
    LIBSSH2_CHANNEL *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_channel_forward_accept(listener);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}


inline ssize_t qssh2_channel_write(LIBSSH2_CHANNEL *&channel, const char *buf, size_t buflen)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_write_ex(channel, 0, buf, buflen);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_flush(LIBSSH2_CHANNEL *&channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_flush(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}


inline int qssh2_channel_send_eof(LIBSSH2_CHANNEL *&channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_send_eof(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_request_pty(LIBSSH2_CHANNEL *&channel, const char *term)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_request_pty_ex(channel, term, static_cast<unsigned int>(strlen(term)), nullptr, 0, LIBSSH2_TERM_WIDTH, LIBSSH2_TERM_HEIGHT, LIBSSH2_TERM_WIDTH_PX, LIBSSH2_TERM_HEIGHT_PX);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}


inline int qssh2_channel_exec(LIBSSH2_CHANNEL *&channel, const char *command)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        if(channel == nullptr)
        {
            return ret;
        }
        ret = libssh2_channel_process_startup(channel, "exec", sizeof("exec") - 1, command, static_cast<unsigned int>(strlen(command)));
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline LIBSSH2_LISTENER * qssh2_channel_forward_listen_ex(LIBSSH2_SESSION *session, const char *host, int port, int *bound_port, int queue_maxsize = 16)
{
    LIBSSH2_LISTENER *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_channel_forward_listen_ex(session, host, port, bound_port, queue_maxsize);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}


inline LIBSSH2_SFTP * qssh2_sftp_init(LIBSSH2_SESSION *session)
{
    LIBSSH2_SFTP *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_sftp_init(session);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}

inline int qssh2_sftp_close_handle(LIBSSH2_SFTP_HANDLE *handle)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_close_handle(handle);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_sftp_shutdown(LIBSSH2_SFTP *sftp)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_shutdown(sftp);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline LIBSSH2_SFTP_HANDLE * qssh2_sftp_open_ex(LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp,
                                                const char *filename,
                                                unsigned int filename_len,
                                                unsigned long flags,
                                                long mode, int open_type)
{
    LIBSSH2_SFTP_HANDLE *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_sftp_open_ex(sftp, filename, filename_len, flags, mode, open_type);
        if(cret == nullptr)
        {
            ret = libssh2_session_last_error(session, nullptr, nullptr, 0);
            if(ret == LIBSSH2_ERROR_EAGAIN)
            {
                QCoreApplication::processEvents();
                continue;
            }
        }
        break;
    }
    return cret;
}

inline ssize_t qssh2_sftp_write(LIBSSH2_SFTP_HANDLE *handle,
                            const char *buffer,
                            size_t count)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_write(handle, buffer, count);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline ssize_t qssh2_sftp_read(LIBSSH2_SFTP_HANDLE *handle,
                                char *buffer, size_t buffer_maxlen)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_read(handle, buffer, buffer_maxlen);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_sftp_mkdir_ex(LIBSSH2_SFTP *sftp,
                               const char *path,
                               unsigned int path_len, long mode)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_mkdir_ex(sftp, path, path_len, mode);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_sftp_readdir_ex(LIBSSH2_SFTP_HANDLE *handle,
                                 char *buffer, size_t buffer_maxlen,
                                 char *longentry, size_t longentry_maxlen,
                                 LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_readdir_ex(handle, buffer, buffer_maxlen, longentry, longentry_maxlen, attrs);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_sftp_stat_ex(LIBSSH2_SFTP *sftp,
                               const char *path,
                               unsigned int path_len,
                               int stat_type,
                               LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_stat_ex(sftp, path, path_len, stat_type, attrs);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline ssize_t qssh2_sftp_unlink_ex(LIBSSH2_SFTP *sftp,
                                    const char *filename,
                                    unsigned int filename_len)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_sftp_unlink_ex(sftp, filename, filename_len);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline QString qssh2_get_last_error(LIBSSH2_SESSION *session)
{
    char *errormsg;
    int errorlen;
    libssh2_session_last_error(session, &errormsg, &errorlen, 0);
    return QString(errormsg);
}
