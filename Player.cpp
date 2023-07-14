#include "Player.h"
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
}

Player::~Player()
{
    if (m_videoPlayer && m_videoPlayer->isRunning())
    {
        m_videoPlayer->terminate();
        m_videoPlayer->wait(100);
        delete m_videoPlayer;m_videoPlayer = NULL;
    }
}
//刷新图片
void Player::slot_refreshImage(QImage img)
{
    ui.lb_video->setPixmap(QPixmap::fromImage(img));
}
//开始按钮,点击之后，从文件中不断获取图片粘贴到控件上
void Player::pb_begin()
{
    //1.开启线程 -> 从文件中不断获取图片粘贴到控件上
    m_videoPlayer->start();
    //2.抛出信号 -> slot_refreshImage
}