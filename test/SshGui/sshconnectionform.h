#ifndef SSHCONNECTIONFORM_H
#define SSHCONNECTIONFORM_H

#include <QWidget>
#include <sshclient.h>


namespace Ui {
class SshConnectionForm;
}

class DirectTunnelForm;

class SshConnectionForm : public QWidget
{
    Q_OBJECT
    SshClient *m_client;

public:
    explicit SshConnectionForm(const QString &name, QWidget *parent = nullptr);
    ~SshConnectionForm();

private slots:
    void on_connectButton_clicked();

private:
    Ui::SshConnectionForm *ui;

private slots:
    void onSshStateChanged(SshClient::SshState sshState);
    void on_createDirectTunnelButton_clicked();
    void destroyChannel(QWidget *ch);
    void on_createReverseTunnelButton_clicked();
};

#endif // SSHCONNECTIONFORM_H
