#pragma once

#include <QDialog>
#include "ui_online_dialog.h"

class online_dialog : public QDialog
{
	Q_OBJECT

public:
	online_dialog(QWidget *parent = nullptr);
	~online_dialog();
public slots:
	//Ã·Ωª
	void pb_send();
signals:
	void SIG_RtmpUrl(QString);

	

private:
	Ui::online_dialogClass ui;
};
