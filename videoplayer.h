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
#include<iostream>
using namespace std;
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
	//音频回调函数使用的量
	uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	unsigned int audio_buf_size = 0;
	unsigned int audio_buf_index = 0;
	//////////////////////////////////////////////////////
	///////////////视频///////////////////////////////////
	AVStream* video_st; //视频流
	PacketQueue* videoq;//视频队列
	AVCodecContext* pCodecCtx;//音频解码器信息指针
	int videoStream;//视频音频流索引
	double video_clock; ///<pts of last decoded frame 视频时钟
	SDL_Thread* video_tid; //视频线程 id
	/////////////////////////////////////////////////////

	//////////播放控制//////////////////////////////////////
	int quit;
	//////////////////////////////////////////////////////
	int64_t start_time; //单位 微秒
	VideoState()
	{
		audio_clock = video_clock = start_time = 0;
	}
	videoplayer* m_player;//用于调用函数
} VideoState;
//线程类：从文件中不断获取图片以信号的形式发送
class videoplayer  : public QThread
{
	Q_OBJECT

public:
	explicit videoplayer(QObject *parent = nullptr);
	~videoplayer();
public:
public:
	//设置文件名
	void setFileName(const QString& fileName);
	//发送图片信号函数
	void SendGetOneImage(QImage& img);
signals:
	void SIG_getOneImage(QImage img);//因为是跨线程，所以不能用引用

private:
	void run();
	//线程执行函数 用start()开启
	QString m_fileName;//文件名



	VideoState m_videoState;

};
