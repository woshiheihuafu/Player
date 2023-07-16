#include "videoSlider.h"
#include<qstyle.h>
#include <qmouseeventtransition.h>
#include <qevent.h>
videoSlider::videoSlider(QWidget* parent) : QSlider(parent)
{}



void videoSlider::mousePressEvent(QMouseEvent* event)
{
	int value = QStyle::sliderValueFromPosition(minimum(), maximum(), event->pos().x(), width());
	setValue(value);
	emit SIG_valueChanged(value);
}
