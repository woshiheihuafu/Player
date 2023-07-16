#pragma once
#include"packetqueue.h"
extern "C"
{
#include"libavcodec/avcodec.h"
#include"libavdevice/avdevice.h"
#include"libswscale/swscale.h"
#include"libavformat/avformat.h"
#include"libavutil/time.h"

#include <SDL.h>
}
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")
#pragma comment(lib,"SDL2test.lib")
#include<qimage.h>
#include<qthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <qmutex.h>
#include<iostream>
using namespace std;
typedef struct {
	time_t lasttime;
	bool connected;
} Runner;
class videoplayer;
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 //1 second of 48khz 32bit audio
typedef struct VideoState
{
	AVFormatContext* pFormatCtx;//相当于视频”文件指针”
	///////////////音频///////////////////////////////////
	AVStream* audio_st; //音频流
	PacketQueue* audioq;//音频缓冲队列
	AVCodecContext* pAudioCodecCtx;//音频解码器信息指针
	int audioStream;//视频音频流索引
	double audio_clock; ///<pts of last decoded frame 音频时钟
	SDL_AudioDeviceID audioID; //音频 ID
	AVFrame out_frame; //设置参数，供音频解码后的 swr_alloc_set_opts 使用。
	AVStream* video_st; //视频流
	PacketQueue* videoq;//视频队列
	AVCodecContext* pCodecCtx;//音频解码器信息指针
	int videoStream;//视频音频流索引
	double video_clock; ///<pts of last decoded frame 视频时钟
	SDL_Thread* video_tid; //视频线程 id
	/////////////////////////////////////////////////////

	//////////播放控制//////////////////////////////////////
	//播放停止
	bool isPause = 0;
	bool isQuit = 0;
	bool readFinished = true;//读取文件是否完毕
	bool readThreadFinished = true;//读取线程是否结束
	bool videoThreadFinished = true;//视频线程是否结束
	//进度控制相关变量
	int seek_req;//跳转标志 -- 读线程
	int64_t seek_pos;//跳转的位置 -- 微妙
	int seek_flag_audio;//跳转标志 -- 用于音频线程中
	int seek_flag_video;//跳转标志 -- 用于视频线程中
	double seek_time;//跳转的时间(秒)--同seek_pos
	//////////////////////////////////////////////////////
	int64_t start_time; //单位 微秒

	//用于网络的起始pts
	bool beginFrame = false;//微秒
	VideoState()
	{
		audio_clock = video_clock = start_time = 0;
	}
	videoplayer* m_player;//用于调用函数
} VideoState;
//播放控制结构体

//播放状态枚举
enum PlayerState
{
	Playing = 0,
	Pause,
	Stop
};
//线程类：从文件中不断获取图片以信号的形式发送
class videoplayer  : public QThread
{
	Q_OBJECT

public:
	explicit videoplayer(QObject *parent = nullptr);
	~videoplayer();
public:
	//获取当前时间
	double getCurrentTime();
	//获取总时间
	int64_t getTotalTime();
	//获取播放状态
	PlayerState getPlayerState() const;
	//跳转函数
	void seek(int64_t pos);
	//播放控制
	void begin();
	void play();
	void pause();
	void stop(bool isWait);
	PlayerState m_playerState = Stop;

public:
	//设置文件名
	void setFileName(const QString& fileName);
	//发送图片信号函数
	void SendGetOneImage(QImage& img);
signals:
	void SIG_getOneImage(QImage img);//因为是跨线程，所以不能用引用
	void SIG_PlayerStateChanged(int PlayerState);
	void SIG_TotalTime(qint64 uSec);//总时间信号
public:
	VideoState m_videoState;
	QMutex mutex;
	int DelayCount = 0;//延时退出
private:
	void run();
	//线程执行函数 用start()开启
	QString m_fileName;//文件名




};
