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
	AVFormatContext* pFormatCtx;//�൱����Ƶ���ļ�ָ�롱
	///////////////��Ƶ///////////////////////////////////
	AVStream* audio_st; //��Ƶ��
	PacketQueue* audioq;//��Ƶ�������
	AVCodecContext* pAudioCodecCtx;//��Ƶ��������Ϣָ��
	int audioStream;//��Ƶ��Ƶ������
	double audio_clock; ///<pts of last decoded frame ��Ƶʱ��
	SDL_AudioDeviceID audioID; //��Ƶ ID
	AVFrame out_frame; //���ò���������Ƶ������ swr_alloc_set_opts ʹ�á�
	//��Ƶ�ص�����ʹ�õ���
	uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	unsigned int audio_buf_size = 0;
	unsigned int audio_buf_index = 0;
	//////////////////////////////////////////////////////
	///////////////��Ƶ///////////////////////////////////
	AVStream* video_st; //��Ƶ��
	PacketQueue* videoq;//��Ƶ����
	AVCodecContext* pCodecCtx;//��Ƶ��������Ϣָ��
	int videoStream;//��Ƶ��Ƶ������
	double video_clock; ///<pts of last decoded frame ��Ƶʱ��
	SDL_Thread* video_tid; //��Ƶ�߳� id
	/////////////////////////////////////////////////////

	//////////���ſ���//////////////////////////////////////
	int quit;
	//////////////////////////////////////////////////////
	int64_t start_time; //��λ ΢��
	VideoState()
	{
		audio_clock = video_clock = start_time = 0;
	}
	videoplayer* m_player;//���ڵ��ú���
} VideoState;
//�߳��ࣺ���ļ��в��ϻ�ȡͼƬ���źŵ���ʽ����
class videoplayer  : public QThread
{
	Q_OBJECT

public:
	explicit videoplayer(QObject *parent = nullptr);
	~videoplayer();
public:
public:
	//�����ļ���
	void setFileName(const QString& fileName);
	//����ͼƬ�źź���
	void SendGetOneImage(QImage& img);
signals:
	void SIG_getOneImage(QImage img);//��Ϊ�ǿ��̣߳����Բ���������

private:
	void run();
	//�߳�ִ�к��� ��start()����
	QString m_fileName;//�ļ���



	VideoState m_videoState;

};
