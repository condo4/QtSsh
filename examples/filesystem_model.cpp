#include "filesystem_model.h"
#include "ui_filesystem_model.h"

#include <qtssh/sshclient.h>
#include <qtssh/sshfilesystemmodel.h>
#include <QFileSystemModel>
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow() : QWidget(), ui(new Ui_MainWindow)
{
	ui->setupUi(this);
	connect(ui->host_edit, SIGNAL(editingFinished()), SLOT(onHostEdited()));

	ui->host_edit->setText("localhost");
	onHostEdited();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onHostEdited()
{
	const QString &host = ui->host_edit->text();
	if (this->host == host) return;

	QAbstractItemModel *model = NULL;
	if (host.isEmpty()) {
		QFileSystemModel *m = new QFileSystemModel(ui->browser);
		m->setRootPath(QDir::currentPath());
		model = m;
	} else {
		SshClient *client = new SshClient(ui->browser);
		qDebug() << "connecting to" << host;
		if (client->connectToHost("rhaschke", host) == 0) {
			SshFilesystemModel *m = new SshFilesystemModel(client);
			model = m;
		}
	}
	this->host = host;

	auto old = ui->browser->model();
	ui->browser->setModel(model);
	if (old) delete old;
}

void MainWindow::reportError(const QString &error) const {
	qWarning() << error;
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainWindow w;
	w.show();
	return app.exec();
}
