#pragma once

#include <QtWidgets/QWidget>
#include<qimage.h>
#include "ui_Player.h"
#include "videoplayer.h"
#include <qtimer.h>
#include "online_dialog.h"
class Player : public QWidget
{
    Q_OBJECT

public:
    Player(QWidget *parent = nullptr);
    ~Player();
public slots:
    //开始按钮
    void pb_begin();
    //停止按钮
    void pb_stop();
    //选择文件
    void pb_file_select();
    //网络播放
    void pb_online();
    //获取总时间
    void slot_getTotalTime(qint64 uSec);
    //刷新图片
    void slot_refreshImage(QImage img);
    //定时器更新当前进度时间
    void slot_TimerTimeOut();
    //状态改变
    void slot_PlayerStateChanged(int state);
    //进度条拖动
    void slot_slider_valueChange(int value);
    //接收url
    void slot_RtmpUrl(QString url);
public:
    int isStop;
    QTimer* m_Timer;
private:
    videoplayer* m_videoPlayer;
    //拉流窗口
    online_dialog* m_onlineDialog;

private:
    Ui::PlayerClass ui;
};
