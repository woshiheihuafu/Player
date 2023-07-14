//#ifndef PACKETQUEUE_H
//#define PACKETQUEUE_H
//#ifdef __cplusplus
//extern "C" 
//{
//#endif
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswresample/swresample.h>
//#include <SDL.h>
//#include"libavdevice/avdevice.h"
//#include"libswscale/swscale.h"
//#include"libavutil/time.h"
//
//#pragma comment(lib,"swscale.lib")
//#pragma comment(lib,"avutil.lib")
//#pragma comment(lib,"avcodec.lib")
//#pragma comment(lib,"avformat.lib")
//#pragma comment(lib,"swresample.lib")
//#pragma comment(lib,"SDL2.lib")
//#pragma comment(lib,"SDL2main.lib")
//#pragma comment(lib,"SDL2test.lib")
//typedef struct PacketQueue
//{
//	AVPacketList* first_pkt; //��ͷ��һ�� packet, ע�����Ͳ��� AVPacket
//	AVPacketList* last_pkt; //��β packet
//	int nb_packets; // paket ����
//	int size; //
//	SDL_mutex* mutex; //
//	SDL_cond* cond; // ��������
//
//}PacketQueue;
//	void packet_queue_init(PacketQueue* queue);
//	int packet_queue_put(PacketQueue* queue, AVPacket* packet);
//	int packet_queue_get(PacketQueue* queue, AVPacket* pakcet, int block);
//#ifdef __cplusplus
//}
//#endif
//#endif // PACKETQUEUE_H
#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL.h>
#include"libavdevice/avdevice.h"
#include"libswscale/swscale.h"
#include"libavutil/time.h"
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")
#pragma comment(lib,"SDL2test.lib")
	typedef struct PacketQueue
	{
		AVPacketList* first_pkt; //��ͷ��һ�� packet, ע�����Ͳ��� AVPacket
		AVPacketList* last_pkt; //��β packet
		int nb_packets; // paket ����
		int size; //
		SDL_mutex* mutex; //
		SDL_cond* cond; // ��������

	}PacketQueue;
	void packet_queue_init(PacketQueue* queue);
	int packet_queue_put(PacketQueue* queue, AVPacket* packet);
	int packet_queue_get(PacketQueue* queue, AVPacket* pakcet, int block);
#ifdef __cplusplus
}
#endif
#endif // PACKETQUEUE_H


