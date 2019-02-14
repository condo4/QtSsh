#pragma once

extern "C" {
#include <libssh2.h>
}

#include <QCoreApplication>

/* Safe ASYNC function */
#define qssh2_knownhost               libssh2_knownhost
#define qssh2_init                      libssh2_init
#define qssh2_session_init_ex           libssh2_session_init_ex
#define qssh2_session_callback_set      libssh2_session_callback_set
#define qssh2_knownhost_init            libssh2_knownhost_init
#define qssh2_session_set_blocking      libssh2_session_set_blocking
#define qssh2_session_hostkey           libssh2_session_hostkey
#define qssh2_hostkey_hash              libssh2_hostkey_hash
#define qssh2_knownhost_check           libssh2_knownhost_check
#define qssh2_session_last_error        libssh2_session_last_error
#define qssh2_userauth_authenticated    libssh2_userauth_authenticated
#define qssh2_keepalive_config          libssh2_keepalive_config
#define qssh2_knownhost_free            libssh2_knownhost_free
#define qssh2_knownhost_readfile        libssh2_knownhost_readfile
#define qssh2_knownhost_writefile       libssh2_knownhost_writefile
#define qssh2_knownhost_add             libssh2_knownhost_add
#define qssh2_session_banner_get        libssh2_session_banner_get
#define qssh2_keepalive_send            libssh2_keepalive_send
#define qssh2_channel_eof               libssh2_channel_eof

inline const char* sshErrorToString(int err)
{
    switch(err)
    {
    case LIBSSH2_ERROR_SOCKET_NONE:
        return "LIBSSH2_ERROR_SOCKET_NONE";
    case LIBSSH2_ERROR_BANNER_SEND:
        return "LIBSSH2_ERROR_BANNER_SEND";
    case LIBSSH2_ERROR_KEX_FAILURE:
        return "LIBSSH2_ERROR_KEX_FAILURE";
    case LIBSSH2_ERROR_SOCKET_SEND:
        return "LIBSSH2_ERROR_SOCKET_SEND";
    case LIBSSH2_ERROR_SOCKET_DISCONNECT:
        return "LIBSSH2_ERROR_SOCKET_DISCONNECT";
    case LIBSSH2_ERROR_PROTO:
        return "LIBSSH2_ERROR_PROTO";
    case LIBSSH2_ERROR_EAGAIN:
        return "LIBSSH2_ERROR_EAGAIN";
    }
    return "Unknown LIBSSH2 ERROR";
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


inline int qssh2_channel_close(LIBSSH2_CHANNEL *channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_close(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_wait_closed(LIBSSH2_CHANNEL *channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_wait_closed(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_free(LIBSSH2_CHANNEL *channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_free(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline ssize_t qssh2_channel_read(LIBSSH2_CHANNEL *channel, char *buf, size_t buflen)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
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

inline LIBSSH2_CHANNEL *qssh2_channel_direct_tcpip(LIBSSH2_SESSION *session, const char *host, int port)
{
    LIBSSH2_CHANNEL *cret = nullptr;
    while(cret == nullptr)
    {
        int ret;

        cret = libssh2_channel_direct_tcpip_ex(session, host, port, "127.0.0.1", 22);
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


inline ssize_t qssh2_channel_write(LIBSSH2_CHANNEL *channel, char *buf, size_t buflen)
{
    ssize_t ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_write_ex(channel, 0, buf, buflen);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_flush(LIBSSH2_CHANNEL *channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_flush(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}


inline int qssh2_channel_send_eof(LIBSSH2_CHANNEL *channel)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_send_eof(channel);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline int qssh2_channel_request_pty(LIBSSH2_CHANNEL *channel, const char *term)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_request_pty_ex(channel, term, static_cast<unsigned int>(strlen(term)), nullptr, 0, LIBSSH2_TERM_WIDTH, LIBSSH2_TERM_HEIGHT, LIBSSH2_TERM_WIDTH_PX, LIBSSH2_TERM_HEIGHT_PX);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}


inline int qssh2_channel_exec(LIBSSH2_CHANNEL *channel, const char *command)
{
    int ret = LIBSSH2_ERROR_EAGAIN;

    while (ret == LIBSSH2_ERROR_EAGAIN)
    {
        ret = libssh2_channel_exec(channel, command);
        if(ret == LIBSSH2_ERROR_EAGAIN)
        {
            QCoreApplication::processEvents();
        }
    }
    return ret;
}

inline LIBSSH2_LISTENER * qssh2_channel_forward_listen_ex(LIBSSH2_SESSION *session, const char *host, int port, int *bound_port, int queue_maxsize)
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
