#pragma once

#include <QtWidgets/QWidget>
#include<qimage.h>
#include "ui_Player.h"
#include "videoplayer.h"
class Player : public QWidget
{
    Q_OBJECT

public:
    Player(QWidget *parent = nullptr);
    ~Player();
public slots:
    //开始按钮
    void pb_begin();
    //刷新图片
    void slot_refreshImage(QImage img);
private:
    videoplayer* m_videoPlayer;
private:
    Ui::PlayerClass ui;
};
