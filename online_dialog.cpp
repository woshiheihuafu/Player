#include "online_dialog.h"
#include<qmessagebox.h>
online_dialog::online_dialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

online_dialog::~online_dialog()
{}
//�ύ
void online_dialog::pb_send()
{
	QString url = ui.le_url->text();
	if (url.isEmpty())
	{
		QMessageBox::about(this, "��ʾ", "�������ַ");
		return;
	}
	Q_EMIT SIG_RtmpUrl(url);
	this->close();

}
