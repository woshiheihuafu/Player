#include "Player.h"
#include "qstring.h"
#include <QFileDialog>
#include "videoplayer.h"
#include<iostream>
using namespace std;
#define FILE_NAME "test.mp4"
Player::Player(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    m_videoPlayer = new videoplayer;
    m_videoPlayer->setFileName(FILE_NAME);
    connect(m_videoPlayer, SIGNAL(SIG_getOneImage(QImage)), this, SLOT(slot_refreshImage(QImage)));
    connect(m_videoPlayer, SIGNAL(SIG_TotalTime(qint64)), this, SLOT(slot_getTotalTime(qint64)));

    //定时器
    m_Timer = new QTimer; //定时器-获取当前视频时间
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(slot_TimerTimeOut()));
    m_Timer->setInterval(500);

    //进度条
    connect(ui.hs_progress, SIGNAL(SIG_valueChanged(int)), this,SLOT(slot_slider_valueChange(int)));


    //url
    m_onlineDialog = new online_dialog;
    connect(m_onlineDialog, SIGNAL(SIG_RtmpUrl(QString)), this, SLOT(slot_RtmpUrl(QString)));
}

Player::~Player()
{
    if (m_videoPlayer && m_videoPlayer->isRunning())
    {
        m_videoPlayer->terminate();
        m_videoPlayer->wait(100);
        delete m_videoPlayer;m_videoPlayer = NULL;
    }
    if (m_videoPlayer)
    {
        m_videoPlayer->stop(true);
    }
    if (m_onlineDialog)
    {
        m_onlineDialog->hide();
        delete m_onlineDialog; m_onlineDialog = NULL;

    }
}
//停止按钮
void Player::pb_stop()
{
    /*if (m_videoPlayer->m_playerState == Pause || m_videoPlayer->m_playerState == Playing)
    {
        m_videoPlayer->m_videoState.isQuit == 1;
    }*/
    if (m_videoPlayer->m_playerState == Pause || m_videoPlayer->m_playerState == Playing)
	{
        QPixmap image("sourceFile/begin.png");
		QIcon icon = QIcon(image);
		ui.pb_begin->setIcon(icon);
		//m_videoPlayer->play();
		m_videoPlayer->stop(true);
        m_Timer->stop();
        ui.hs_progress->setValue(0);
        ui.lb_endtime->setText("00:00:00");
        ui.lb_time->setText("00:00:00");
        //ui.lb_videoName->setText("");
        this->update();
        isStop = true;
	}
}
//选择文件
void Player::pb_file_select()
{
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Open File", QDir::homePath(), "Text files (*.txt);;All files (*.*)");
    m_videoPlayer->setFileName(filePath);
}
//网络播放
void Player::pb_online()
{
    m_onlineDialog->show();
}
//获取总时间
void Player::slot_getTotalTime(qint64 uSec)
{
    qint64 Sec = uSec / 1000000;
    ui.hs_progress->setRange(0, Sec);//精确到秒
    QString hStr = QString("00%1").arg(Sec / 3600);
    QString mStr = QString("00%1").arg(Sec / 60);
    QString sStr = QString("00%1").arg(Sec % 60);
    QString str =
        QString("%1:%2:%3").arg(hStr.right(2)).arg(mStr.right(2)).arg(sStr.right(2));
    ui.lb_endtime->setText(str);
}
//刷新图片
void Player::slot_refreshImage(QImage img)
{
    ui.lb_video->setPixmap(QPixmap::fromImage(img));
}
//定时器更新当前进度时间
void Player::slot_TimerTimeOut()
{
    //定时器更新当前进度时间
    if (QObject::sender() == m_Timer)
    {
        qint64 Sec = m_videoPlayer->getCurrentTime() / 1000000;
        ui.hs_progress->setValue(Sec);
        QString hStr = QString("00%1").arg(Sec / 3600);
        QString mStr = QString("00%1").arg(Sec / 60 % 60);
        QString sStr = QString("00%1").arg(Sec % 60);
        QString str =
            QString("%1:%2:%3").arg(hStr.right(2)).arg(mStr.right(2)).arg(sStr.right(2));
        ui.lb_time->setText(str);
        if (ui.hs_progress->value() == ui.hs_progress->maximum()
            && m_videoPlayer->m_playerState == PlayerState::Stop)
        {
            slot_PlayerStateChanged(PlayerState::Stop);
        }
        else if (ui.hs_progress->value() + 1 ==
            ui.hs_progress->maximum()
            && m_videoPlayer->m_playerState == PlayerState::Stop)
        {
            slot_PlayerStateChanged(PlayerState::Stop);
        }
    }
}
//开始按钮,点击之后，从文件中不断获取图片粘贴到控件上
void Player::pb_begin()
{
    m_videoPlayer->mutex.lock();
    //1.开启线程 -> 从文件中不断获取图片粘贴到控件上
    if (m_videoPlayer->m_playerState == Pause)
    {
        /*m_videoPlayer->start();
        m_videoPlayer->m_videoState.isQuit = 0;
        m_videoPlayer->m_playerState = Playing;*/
        QPixmap image("sourceFile/pause.png");
        QIcon icon = QIcon(image);
        ui.pb_begin->setIcon(icon);
        m_videoPlayer->play();
        //m_videoPlayer->start();
        m_Timer->start();

    }
    //暂停
    else if (m_videoPlayer->m_playerState == Playing)
    {
        /*m_videoPlayer->m_videoState.isPause = 1;
        m_videoPlayer->m_playerState = Pause;*/
        QPixmap image("sourceFile/begin.png");
        QIcon icon = QIcon(image);
        ui.pb_begin->setIcon(icon);
        m_videoPlayer->pause();
        m_Timer->stop();
    }
    //恢复
    //else if (m_videoPlayer->m_playerState == Pause)
    //{
    //    /*m_videoPlayer->m_videoState.isPause = 0;
    //    m_videoPlayer->m_playerState = Playing;*/
    //    QPixmap image("sourceFile/pause.png");
    //    QIcon icon = QIcon(image);
    //    ui.pb_begin->setIcon(icon);
    //    m_videoPlayer->play();
    //    m_Timer->start();
    //}
    //恢复
    else if (m_videoPlayer->m_playerState == Stop)
    {
        QPixmap image("sourceFile/pause.png");
        QIcon icon = QIcon(image);
        ui.pb_begin->setIcon(icon);
        m_videoPlayer->begin();
        m_Timer->start();
    }
    m_videoPlayer->mutex.unlock();
    /*m_videoPlayer->start();*/
}

void Player::slot_PlayerStateChanged(int state)
{
	switch (state)
	{
	case PlayerState::Stop:
        {
        QPixmap image("sourceFile/begin.png");
        QIcon icon = QIcon(image);
        ui.pb_begin->setIcon(icon);
        }
        m_videoPlayer->stop(true);
        m_Timer->stop();
		ui.hs_progress->setValue(0);
		ui.lb_endtime->setText("00:00:00");
		ui.lb_time->setText("00:00:00");
		//ui.lb_videoName->setText("");
		this->update();
		isStop = true;
		break;
	case PlayerState::Playing:
		m_Timer->start();
		this->update();
		isStop = false;
		break;
	}
}
//进度条拖动
void Player::slot_slider_valueChange(int value)
{
    if (QObject::sender() == ui.hs_progress)
    {
        m_videoPlayer->seek((qint64)value * 1000000); //value 秒
    }

}
//接收url
void Player::slot_RtmpUrl(QString url)
{
    m_videoPlayer->stop(true);
    m_videoPlayer->setFileName(url);
}
