#include "reversetunnelform.h"
#include "ui_reversetunnelform.h"
#include <sshtunnelin.h>
#include <sshclient.h>
#include <QMetaEnum>


int ReverseTunnelForm::m_counter = 0;

ReverseTunnelForm::ReverseTunnelForm(SshClient *client,  quint16 listenPort, QString hostTarget, quint16 portTarget, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReverseTunnelForm)
{
    ui->setupUi(this);
    m_tunnel = client->getChannel<SshTunnelIn>(QString("tunnel_%1").arg(m_counter++));
    m_tunnel->listen(hostTarget, portTarget, listenPort);
    QObject::connect(m_tunnel, &SshTunnelIn::connectionChanged, this, &ReverseTunnelForm::connectionChanged);
    QObject::connect(m_tunnel, &SshTunnelIn::stateChanged, this, &ReverseTunnelForm::stateChanged);

    ui->txtTarget->setText(QString("%1:%2").arg(hostTarget).arg(portTarget));
    ui->txtConnections->setText("0");
    ui->txtState->setText(QMetaEnum::fromType<SshChannel::ChannelState>().valueToKey( int(m_tunnel->channelState()) ));
}

ReverseTunnelForm::~ReverseTunnelForm()
{
    delete ui;
}

void ReverseTunnelForm::connectionChanged(int connections)
{
    ui->txtConnections->setText(QString("%1").arg(connections));
}

void ReverseTunnelForm::on_closeButton_clicked()
{
    switch(m_tunnel->channelState()) {
    case  SshChannel::ChannelState::Read:
        m_tunnel->close();
        ui->closeButton->setText("Remove");
        break;
    case SshChannel::ChannelState::Free:
        qWarning() << "Ask to destroy";
        emit destroyme(this);
        break;
    default:
        qWarning() << "Click remove when state is " << QMetaEnum::fromType<SshChannel::ChannelState>().valueToKey( int(m_tunnel->channelState()) ) << "(" << m_tunnel->channelState() << ")";
        break;
    }
}

void ReverseTunnelForm::stateChanged(SshChannel::ChannelState state)
{
    ui->txtLocalPort->setText(QString("%1").arg(m_tunnel->remotePort()));
    ui->txtState->setText(QMetaEnum::fromType<SshChannel::ChannelState>().valueToKey( int(state) ));
}
