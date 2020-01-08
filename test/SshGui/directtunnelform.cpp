#include "directtunnelform.h"
#include "ui_directtunnelform.h"
#include <sshtunnelout.h>
#include <sshclient.h>
#include <QMetaEnum>


int DirectTunnelForm::m_counter = 0;

DirectTunnelForm::DirectTunnelForm(SshClient *client, QString hosttarget, QString hostlisten, quint16 port, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DirectTunnelForm)
{
    ui->setupUi(this);
    m_tunnel = client->getChannel<SshTunnelOut>(QString("tunnel_%1").arg(m_counter++));
    m_tunnel->listen(port, hosttarget, hostlisten);
    QObject::connect(m_tunnel, &SshTunnelOut::connectionChanged, this, &DirectTunnelForm::connectionChanged);
    QObject::connect(m_tunnel, &SshTunnelOut::stateChanged, this, &DirectTunnelForm::stateChanged);
    QObject::connect(m_tunnel, &SshTunnelOut::destroyed, this, &DirectTunnelForm::destroyme);

    ui->txtLocalPort->setText(QString("%1").arg(m_tunnel->localPort()));
    ui->txtRemotePort->setText(QString("%1").arg(m_tunnel->port()));
    ui->txtConnections->setText("0");
    ui->txtState->setText(QMetaEnum::fromType<SshChannel::ChannelState>().valueToKey( int(m_tunnel->channelState()) ));
}

DirectTunnelForm::~DirectTunnelForm()
{
    delete ui;
}

void DirectTunnelForm::connectionChanged(int connections)
{
    ui->txtConnections->setText(QString("%1").arg(connections));
    if(m_tunnel && connections == 0)
    {
        stateChanged(m_tunnel->channelState());
    }
}

void DirectTunnelForm::on_closeButton_clicked()
{
    if(!m_tunnel) return;

    switch(m_tunnel->channelState()) {
    case  SshChannel::ChannelState::Ready:
        m_tunnel->close();
        ui->closeButton->setText("Remove");
        break;
    case SshChannel::ChannelState::Free:
        qWarning() << "Ask to destroy";
        if(m_tunnel->connections() > 0)
        {
            m_tunnel->closeAllConnections();
        }
        break;
    default:
        qWarning() << "Click remove when state is " << QMetaEnum::fromType<SshChannel::ChannelState>().valueToKey( int(m_tunnel->channelState()) ) << "(" << m_tunnel->channelState() << ")";
        break;
    }
}

void DirectTunnelForm::stateChanged(SshChannel::ChannelState state)
{
    ui->txtState->setText(QMetaEnum::fromType<SshChannel::ChannelState>().valueToKey( int(state) ));
}
