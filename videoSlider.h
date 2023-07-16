#pragma once
#include <qmouseeventtransition.h>
#include <QObject>
#include <qslider.h>
class videoSlider : public QSlider
{
	Q_OBJECT
public:
	explicit videoSlider(QWidget* parent = 0);
signals:
	void SIG_valueChanged(int);
private:
	void mousePressEvent(QMouseEvent* event);
};