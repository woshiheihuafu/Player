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
    //��ʼ��ť
    void pb_begin();
    //ֹͣ��ť
    void pb_stop();
    //ѡ���ļ�
    void pb_file_select();
    //���粥��
    void pb_online();
    //��ȡ��ʱ��
    void slot_getTotalTime(qint64 uSec);
    //ˢ��ͼƬ
    void slot_refreshImage(QImage img);
    //��ʱ�����µ�ǰ����ʱ��
    void slot_TimerTimeOut();
    //״̬�ı�
    void slot_PlayerStateChanged(int state);
    //�������϶�
    void slot_slider_valueChange(int value);
    //����url
    void slot_RtmpUrl(QString url);
public:
    int isStop;
    QTimer* m_Timer;
private:
    videoplayer* m_videoPlayer;
    //��������
    online_dialog* m_onlineDialog;

private:
    Ui::PlayerClass ui;
};
