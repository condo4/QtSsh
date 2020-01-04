#ifndef DIRECTTUNNELFORM_H
#define DIRECTTUNNELFORM_H

#include <QWidget>
#include <sshchannel.h>
class SshClient;
class SshTunnelOut;

namespace Ui {
class DirectTunnelForm;
}

class DirectTunnelForm : public QWidget
{
    Q_OBJECT
    SshTunnelOut *m_tunnel;
    static int m_counter;

public:
    explicit DirectTunnelForm(SshClient *client, QString hosttarget, QString hostlisten, quint16 port, QWidget *parent = nullptr);
    ~DirectTunnelForm();

private:
    Ui::DirectTunnelForm *ui;

private slots:
    void connectionChanged(int);
    void on_closeButton_clicked();
    void stateChanged(SshChannel::ChannelState state);

signals:
    void destroyme();
};

#endif // DIRECTTUNNELFORM_H
