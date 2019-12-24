#include "sshconnectionform.h"
#include "ui_sshconnectionform.h"
#include "directtunnelform.h"
#include "reversetunnelform.h"
#include <QMetaEnum>


SshConnectionForm::SshConnectionForm(const QString &name, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SshConnectionForm)
{
    ui->setupUi(this);
    m_client = new SshClient(name, this);
    QObject::connect(m_client, &SshClient::sshStateChanged, this, &SshConnectionForm::onSshStateChanged);
}

SshConnectionForm::~SshConnectionForm()
{
    delete ui;
}

void SshConnectionForm::on_connectButton_clicked()
{
    if((m_client->sshState() == SshClient::SshState::Unconnected))
    {
        m_client->setPassphrase(ui->passwordEdit->text());
        m_client->connectToHost(ui->loginEdit->text(), ui->hostnameEdit->text(), ui->portEdit->text().toUShort());
    }
    else
    {
        m_client->disconnectFromHost();
    }
}

void SshConnectionForm::onSshStateChanged(SshClient::SshState sshState)
{
    qWarning() << "New SSH State: " << sshState;
    ui->sshStateText->setText(QMetaEnum::fromType<SshClient::SshState>().valueToKey( int(sshState) ));
    ui->connectButton->setText((sshState == SshClient::SshState::Unconnected)?("Connect"):("Disconnect"));
    ui->actionTabWidget->setEnabled(sshState == SshClient::SshState::Ready);
}

void SshConnectionForm::on_createDirectTunnelButton_clicked()
{
    if(ui->portInput->text().length() > 0)
    {
        DirectTunnelForm *form = new DirectTunnelForm(m_client, ui->hostInput->text(), "0.0.0.0", ui->portInput->text().toUShort(), this);
        QObject::connect(form, &DirectTunnelForm::destroyme, this, &SshConnectionForm::destroyChannel);
        ui->listTunnels->layout()->addWidget(form);
    }
}

void SshConnectionForm::destroyChannel(QWidget *ch)
{
    qWarning() << "Remove channel" << ch;
    ui->listTunnels->layout()->removeWidget(ch);
    ch->deleteLater();
}

void SshConnectionForm::on_createReverseTunnelButton_clicked()
{
    if(ui->rtargetPortInput->text().length() > 0)
    {
        ReverseTunnelForm *form = new ReverseTunnelForm(m_client, ui->rListenPortInput->text().toUShort(), ui->rhostInput->text(), ui->rtargetPortInput->text().toUShort(), this);
        QObject::connect(form, &ReverseTunnelForm::destroyme, this, &SshConnectionForm::destroyChannel);
        ui->listTunnels->layout()->addWidget(form);
    }
}
