#pragma once

#include <QWidget>

class Ui_MainWindow;
class MainWindow : public QWidget {
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

public Q_SLOTS:
	void onHostEdited();
	void reportError(const QString &error) const;

private:
	Ui_MainWindow *ui;
	QString host;
};
