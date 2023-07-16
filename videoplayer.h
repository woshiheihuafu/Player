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
	AVFormatContext* pFormatCtx;//�൱����Ƶ���ļ�ָ�롱
	///////////////��Ƶ///////////////////////////////////
	AVStream* audio_st; //��Ƶ��
	PacketQueue* audioq;//��Ƶ�������
	AVCodecContext* pAudioCodecCtx;//��Ƶ��������Ϣָ��
	int audioStream;//��Ƶ��Ƶ������
	double audio_clock; ///<pts of last decoded frame ��Ƶʱ��
	SDL_AudioDeviceID audioID; //��Ƶ ID
	AVFrame out_frame; //���ò���������Ƶ������ swr_alloc_set_opts ʹ�á�
	AVStream* video_st; //��Ƶ��
	PacketQueue* videoq;//��Ƶ����
	AVCodecContext* pCodecCtx;//��Ƶ��������Ϣָ��
	int videoStream;//��Ƶ��Ƶ������
	double video_clock; ///<pts of last decoded frame ��Ƶʱ��
	SDL_Thread* video_tid; //��Ƶ�߳� id
	/////////////////////////////////////////////////////

	//////////���ſ���//////////////////////////////////////
	//����ֹͣ
	bool isPause = 0;
	bool isQuit = 0;
	bool readFinished = true;//��ȡ�ļ��Ƿ����
	bool readThreadFinished = true;//��ȡ�߳��Ƿ����
	bool videoThreadFinished = true;//��Ƶ�߳��Ƿ����
	//���ȿ�����ر���
	int seek_req;//��ת��־ -- ���߳�
	int64_t seek_pos;//��ת��λ�� -- ΢��
	int seek_flag_audio;//��ת��־ -- ������Ƶ�߳���
	int seek_flag_video;//��ת��־ -- ������Ƶ�߳���
	double seek_time;//��ת��ʱ��(��)--ͬseek_pos
	//////////////////////////////////////////////////////
	int64_t start_time; //��λ ΢��

	//�����������ʼpts
	bool beginFrame = false;//΢��
	VideoState()
	{
		audio_clock = video_clock = start_time = 0;
	}
	videoplayer* m_player;//���ڵ��ú���
} VideoState;
//���ſ��ƽṹ��

//����״̬ö��
enum PlayerState
{
	Playing = 0,
	Pause,
	Stop
};
//�߳��ࣺ���ļ��в��ϻ�ȡͼƬ���źŵ���ʽ����
class videoplayer  : public QThread
{
	Q_OBJECT

public:
	explicit videoplayer(QObject *parent = nullptr);
	~videoplayer();
public:
	//��ȡ��ǰʱ��
	double getCurrentTime();
	//��ȡ��ʱ��
	int64_t getTotalTime();
	//��ȡ����״̬
	PlayerState getPlayerState() const;
	//��ת����
	void seek(int64_t pos);
	//���ſ���
	void begin();
	void play();
	void pause();
	void stop(bool isWait);
	PlayerState m_playerState = Stop;

public:
	//�����ļ���
	void setFileName(const QString& fileName);
	//����ͼƬ�źź���
	void SendGetOneImage(QImage& img);
signals:
	void SIG_getOneImage(QImage img);//��Ϊ�ǿ��̣߳����Բ���������
	void SIG_PlayerStateChanged(int PlayerState);
	void SIG_TotalTime(qint64 uSec);//��ʱ���ź�
public:
	VideoState m_videoState;
	QMutex mutex;
	int DelayCount = 0;//��ʱ�˳�
private:
	void run();
	//�߳�ִ�к��� ��start()����
	QString m_fileName;//�ļ���




};
