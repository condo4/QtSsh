#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sshconnectionform.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    SshConnectionForm *new_ssh = new SshConnectionForm(ui->conname->text(), this);
    ui->tabWidget->addTab(new_ssh, ui->conname->text());
}
