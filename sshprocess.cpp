#include "sshprocess.h"
#include "sshclient.h"
#include <QTimer>
#include <QEventLoop>

SshProcess::SshProcess(SshClient *client) : SshChannel(client)
{
    _isEOF = false;
    _isCommand = false;
    _currentState = OpenChannelSession;
    sshDataReceived();
}

SshProcess::~SshProcess()
{
    stopChannel();
}

QString SshProcess::result()
{
    QByteArray buffer;
    QTimer singleshotTimer;
    QEventLoop loop;
    singleshotTimer.setInterval(2 * 60 * 1000);
    singleshotTimer.setSingleShot(true);
    QObject::connect(this, &SshProcess::connected, &loop, &QEventLoop::quit);
    QObject::connect(this, &SshProcess::readyRead, &loop, &QEventLoop::quit);

    while(!_isEOF)
    {
        QByteArray result;
        qint64 readBytes = 0;
        qint64 readResult;

        singleshotTimer.start();
        loop.exec();

        do {
            result.resize(result.size() + 16*1024);
            readResult = readData(result.data() + readBytes, result.size() - readBytes);
            if (readResult > 0 || readBytes == 0)
                readBytes += readResult;
        } while (readResult > 0);

        if (readBytes <= 0)
            result.clear();
        else
            result.resize(int(readBytes));


        if (!result.isEmpty())
        {
            buffer.append(result);
        }
    }
    singleshotTimer.stop();

#if defined(DEBUG_SSHCHANNEL)
    qDebug() << "DEBUG : SshProcess : Process return result";
#endif
    return QString(buffer);
}

qint64 SshProcess::readData(char *buff, qint64 len)
{
    ssize_t ret = 0;
    ssize_t readbuf = 0;
    bool eof = false;

    if (sshClient->session() == NULL)
    {
        qDebug() << "WARNING : QtSshChannel::readData Process when d_channel == NULL";
        return -EAGAIN;
    }
    if(!_channelInUse.tryAcquire())
    {
        ret = -EAGAIN;
    }
    else
    {
        do
        {
            ret = libssh2_channel_read(sshChannel, buff + readbuf, len - readbuf);
            if(ret > 0) readbuf += ret;
        } while((ret > 0) && (readbuf < len));

        if (libssh2_channel_eof(sshChannel) == 1)
        {
            eof = true;
            if (_isCommand)
            {
                _currentState = NoState;
            }
        }

        _channelInUse.release();

        if (ret < 0)
        {
            // gestion erreurs
            if (ret != LIBSSH2_ERROR_EAGAIN)
            {
                qDebug() << "ERROR : QtSshChannel::tryToRead Process : err=" << ret;
                char * errmsg;
                int errlen;

                int err = libssh2_session_last_error(sshClient->session(), &errmsg, &errlen, 0);
                if (err == LIBSSH2_ERROR_CHANNEL_CLOSED)
                {
                    _currentState = ErrorNoRetry;
                }
            }
        }


        _isEOF = eof;

        if (readbuf > 0) emit data_rx(readbuf);
        ret = readbuf;
    }
    return ret;
}


void SshProcess::sshDataReceived()
{
    switch (_currentState)
    {
        case OpenChannelSession:
        {
            sshChannel=libssh2_channel_open_session(sshClient->session());
            if (sshChannel == NULL)
            {
                if (libssh2_session_last_error(sshClient->session(), NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN)
                {
                    return;
                }
            }
            else
            {
                _channelInUse.release();
#if defined(DEBUG_SSHCHANNEL)
                qDebug() << "DEBUG : QtSshChannel : channel session opened";
#endif
                _currentState = EarlyCmdTransition;
                return sshDataReceived();
            }
            break;
        }
        case EarlyCmdTransition:
        {
            if (!_nextActions.isEmpty())
            {
                _currentState = _nextActions.takeFirst();
                return sshDataReceived();
            }
            else
            {
                return;
            }
            break;
        }
        case RequestPty:
        {
            if(sshChannel != NULL)
            {
                int ret = libssh2_channel_request_pty(sshChannel, "");
                if (ret)
                {
                    if (ret == LIBSSH2_ERROR_EAGAIN)
                    {
                        return;
                    }
                    else
                    {
                        qDebug() << "ERROR : QtSshChannel : pty allocation failed";
                        return;
                    }
                }
                else
                {
#if defined(DEBUG_SSHCHANNEL)
                    qDebug() << "DEBUG : QtSshChannel : pty opened";
#endif
                    _currentState = EarlyCmdTransition;
                    return sshDataReceived();
                }
            }
            else
            {
                qDebug() << "WARNING : QtSshChannel : Channel null for RequestPty";
            }
            break;
        }
        case StartCmdProcess:
        {
            if(sshChannel != NULL)
            {
                int ret = libssh2_channel_exec(sshChannel, qPrintable(_cmd));
                if (ret)
                {
                    if (ret == LIBSSH2_ERROR_EAGAIN)
                    {
                        return;
                    }
                    else
                    {
                        qDebug() << "ERROR : QtSshChannel : process exec failed";
                        return;
                    }
                }
                else
                {
#if defined(DEBUG_SSHCHANNEL)
                    qDebug() << "DEBUG : QtSshChannel : process exec opened";
#endif
                    //setOpenMode(QIODevice::ReadWrite);
                    _isCommand = true;
                    //d_state = 66;
                    _currentState = ReadyRead;
                    emit connected();
                    return;
                }
            }
            else
            {
                qDebug() << "WARNING : QtSshChannel : Channel null for StartCmdProcess";
            }
            break;
        }
        case ReadyRead:
        {
            emit readyRead();
            return;
        }
        case ErrorNoRetry:
        default:
        {
            return;
        }
    }
}

void SshProcess::start(QString cmd)
{
    if (_currentState > RequestPty)
    {
        return;
    }
    _cmd = cmd;
    if (_currentState == EarlyCmdTransition)
    {
        _currentState = StartCmdProcess;
        sshDataReceived();
    }
    else
    {
        if (!_nextActions.contains(StartCmdProcess))
        {
            _nextActions.append(StartCmdProcess);
        }
    }
}
