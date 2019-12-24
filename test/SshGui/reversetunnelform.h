#ifndef REVERSETUNNELFORM_H
#define REVERSETUNNELFORM_H

#include <QWidget>
#include <sshchannel.h>
class SshClient;
class SshTunnelIn;

namespace Ui {
class ReverseTunnelForm;
}

class ReverseTunnelForm : public QWidget
{
    Q_OBJECT
    SshTunnelIn *m_tunnel;
    static int m_counter;

public:
    explicit ReverseTunnelForm(SshClient *client,  quint16 listenPort, QString hostTarget, quint16 hostPort, QWidget *parent = nullptr);
    ~ReverseTunnelForm();

private:
    Ui::ReverseTunnelForm *ui;

private slots:
    void connectionChanged(int);
    void on_closeButton_clicked();
    void stateChanged(SshChannel::ChannelState state);

signals:
    void destroyme(ReverseTunnelForm *);
};

#endif // REVERSETUNNELFORM_H
