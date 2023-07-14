//#include <iostream>
//using namespace std;
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>
//#ifdef __cplusplus
//extern "C"
//{
//#endif
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswresample/swresample.h>
//#include <SDL.h>
//#ifdef __cplusplus
//}
//#endif
//#include "packetqueue.h"
//#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 //1 second of 48khz 32bit audio
//#define SDL_AUDIO_BUFFER_SIZE 1024 //
//#define FILE_NAME "test.mp4"
//#define ERR_STREAM stderr
//#define OUT_SAMPLE_RATE 44100
//AVFrame wanted_frame;
//PacketQueue audio_queue;
//int quit = 0;
////�ص�����
//void audio_callback(void* userdata, Uint8* stream, int len);
////���뺯��
//int audio_decode_frame(AVCodecContext* pcodec_ctx, uint8_t* audio_buf, int buf_size);
////�� auto_stream
//int find_stream_index(AVFormatContext* pformat_ctx, int* video_stream, int
//	* audio_stream);
//#undef main
//int main(int argc, char* argv[])
//{
//	//0.�������
//	//AV �ļ���Ƶ���ġ��ļ�ָ�롱
//	AVFormatContext* pFormatCtx = NULL;
//	int audioStream = -1;//��������Ҫ����������
//	AVCodecContext* pCodecCtx = NULL;//������
//	AVCodec* pCodec = NULL; //������
//	AVPacket packet; // ����ǰ������
//	AVFrame* pframe = NULL; //����֮�������
//	char filename[256] = FILE_NAME;
//	//SDL
//	SDL_AudioSpec wanted_spec; //SDL ��Ƶ����
//	SDL_AudioSpec spec; //SDL ��Ƶ����
//	//1.ffmpeg ��ʼ��
//	av_register_all();
//	//2.SDL ��ʼ��
//	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
//	{
//		fprintf(ERR_STREAM, "Couldn't init SDL:%s\n", SDL_GetError());
//		exit(-1);
//	}
//	//3.���ļ�
//	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
//	{
//		fprintf(ERR_STREAM, "Couldn't open input file\n");
//		exit(-1);
//	}
//	//3.1 ��ȡ�ļ�����Ϣ
//	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
//	{
//		fprintf(ERR_STREAM, "Not Found Stream Info\n");
//		exit(-1);
//	}
//	//��ʾ�ļ���Ϣ��ʮ�ֺ��õ�һ������
//	av_dump_format(pFormatCtx, 0, filename, false);
//	//4.��ȡ��Ƶ��
//	if (find_stream_index(pFormatCtx, NULL, &audioStream) == -1)
//	{
//		fprintf(ERR_STREAM, "Couldn't find stream index\n");
//		exit(-1);
//	}
//	printf("audio_stream = %d\n", audioStream);
//	//5.�ҵ���Ӧ�Ľ�����
//	pCodecCtx = pFormatCtx->streams[audioStream]->codec;
//	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
//	if (!pCodec)
//	{
//		fprintf(ERR_STREAM, "Couldn't find decoder\n");
//		exit(-1);
//	}
//	//6.������Ƶ��Ϣ, ��������Ƶ�豸��
//	wanted_spec.freq = pCodecCtx->sample_rate;
//	wanted_spec.format = AUDIO_S16SYS;
//	wanted_spec.channels = pCodecCtx->channels; //ͨ����
//	wanted_spec.silence = 0; //���þ���ֵ
//	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE; //��ȡ��һ֡�����
//	wanted_spec.callback = audio_callback;//�ص�����
//	wanted_spec.userdata = pCodecCtx;//�ص���������
//	//wanted_spec:��Ҫ�򿪵�
//	//spec:ʵ�ʴ򿪵�,���Բ��������������ֱ���� NULL,�����õ� spec �� wanted_spec ���档
//	//����Ὺһ���̣߳����� callback��
//   //SDL_OpenAudioDevice->open_audio_device(���߳�)->SDL_RunAudio->fill(ָ�� callback ����)
//   //7.����Ƶ�豸��
//	SDL_AudioDeviceID id =
//		SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), 0, &wanted_spec, &spec, 0);
//	if (id < 0) //�ڶ��δ� audio �᷵��-1
//	{
//		fprintf(ERR_STREAM, "Couldn't open Audio: %s\n", SDL_GetError());
//		exit(-1);
//	}
//	//8.���ò�����������ʱ����, swr_alloc_set_opts �� in ���ֲ���
//	wanted_frame.format = AV_SAMPLE_FMT_S16;
//	wanted_frame.sample_rate = spec.freq;
//	wanted_frame.channel_layout = av_get_default_channel_layout(spec.channels);
//	wanted_frame.channels = spec.channels;
//	//9.�򿪽�����, ��ʼ�� AVCondecContext���Լ�����һЩ��������
//	avcodec_open2(pCodecCtx, pCodec, NULL);
//	//10.��ʼ������
//	packet_queue_init(&audio_queue);
//	//11. SDL �������� 0 ����
//	SDL_PauseAudioDevice(id, 0);
//	//12.ѭ����ȡ��Ƶ֡(��һ֡����)������Ƶͬ������
//	while (av_read_frame(pFormatCtx, &packet) >= 0) //��һ�� packet�����ݷ��� packet.data
//	{
//		if (packet.stream_index == audioStream)
//		{
//			packet_queue_put(&audio_queue, &packet);
//		}
//		else
//		{
//			av_free_packet(&packet);
//		}
//	}
//	while (audio_queue.nb_packets != 0)
//	{
//		SDL_Delay(100);
//	}
//	//���տռ�
//	avcodec_close(pCodecCtx);
//	avformat_close_input(&pFormatCtx);
//	printf("play finished\n");
//	return 0;
//}
////ע�� userdata ��ǰ��� AVCodecContext.
////len ��ֵ��Ϊ 2048,��ʾһ�η��Ͷ��١�
////audio_buf_size��һֱΪ�����������Ĵ�С��wanted_spec.samples.��һ��ÿ�ν�����ô�࣬�ļ���ͬ�����ֵ��ͬ)
////audio_buf_index�� ��Ƿ��͵������ˡ�
////��������Ĺ���ģʽ��:
////1. �������ݷŵ� audio_buf, ��С�� audio_buf_size��(audio_buf_size = audio_size;������ã�
////2. ����һ�� callback ֻ�ܷ��� len ���ֽ�,��ÿ��ȡ�صĽ������ݿ��ܱ� len ��һ�η����ꡣ
////3. �������ʱ�򣬻� len == 0��������ѭ�����˳��������������� callback��������һ�η��͡�
////4. �����ϴ�û���꣬��β�ȡ���ݣ����ϴε�ʣ��ģ�audio_buf_size ��Ƿ��͵������ˡ�
////5. ע�⣬callback ÿ��һ��Ҫ���ҽ��� len �����ݣ����򲻻��˳���
////���û��������������û���ˣ�����ȡ�������ˣ����˳���������һ�������Դ�ѭ����
////������������Ϊ static ����Ϊ�˱����ϴ����ݣ�Ҳ������ȫ�ֱ��������Ǹо��������á�
////13.�ص������н��Ӷ�����ȡ����, �������䵽���Ż�����.
//void audio_callback(void* userdata, Uint8* stream, int len)
//{
//	AVCodecContext* pcodec_ctx = (AVCodecContext*)userdata;
//	int len1, audio_data_size;
//	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
//	static unsigned int audio_buf_size = 0;
//	static unsigned int audio_buf_index = 0;
//	/* len ���� SDL ����� SDL �������Ĵ�С������������δ�������Ǿ�һֱ����������� */
//	/* audio_buf_index �� audio_buf_size ��ʾ�����Լ��������ý�����������ݵĻ�������*/
//	/* ��Щ���ݴ� copy �� SDL �������� �� audio_buf_index >= audio_buf_size ��ʱ����ζ����*/
//	/* �ǵĻ���Ϊ�գ�û�����ݿɹ� copy����ʱ����Ҫ���� audio_decode_frame ���������
//	/* ��������� */
//	while (len > 0)
//	{
//		if (audio_buf_index >= audio_buf_size) {
//			audio_data_size = audio_decode_frame(pcodec_ctx,
//				audio_buf, sizeof(audio_buf));
//			/* audio_data_size < 0 ��ʾû�ܽ�������ݣ�����Ĭ�ϲ��ž��� */
//			if (audio_data_size < 0) {
//				/* silence */
//				audio_buf_size = 1024;
//				/* ���㣬���� */
//				memset(audio_buf, 0, audio_buf_size);
//			}
//			else {
//				audio_buf_size = audio_data_size;
//			}
//			audio_buf_index = 0;
//		}
//		/* �鿴 stream ���ÿռ䣬����һ�� copy �������ݣ�ʣ�µ��´μ��� copy */
//		len1 = audio_buf_size - audio_buf_index;
//		if (len1 > len) {
//			len1 = len;
//		}
//		//SDL_MixAudio ��������
//		memcpy(stream, (uint8_t*)audio_buf + audio_buf_index, len1);
//		len -= len1;
//		stream += len1;
//		audio_buf_index += len1;
//	}
//}
////������Ƶ��˵��һ�� packet ���棬���ܺ��ж�֡(frame)���ݡ�
//int audio_decode_frame(AVCodecContext* pcodec_ctx, uint8_t* audio_buf, int buf_size)
//{
//	static AVPacket pkt;
//	static uint8_t* audio_pkt_data = NULL;
//	static int audio_pkt_size = 0;
//	int len1, data_size;
//	int sampleSize = 0;
//	AVCodecContext* aCodecCtx = pcodec_ctx;
//	AVFrame* audioFrame = NULL;
//	PacketQueue* audioq = &audio_queue;
//	static struct SwrContext* swr_ctx = NULL;
//	int convert_len;
//	int n = 0;
//	for (;;)
//	{
//		if (quit) return -1;
//		if (packet_queue_get(audioq, &pkt, 0) <= 0) //һ��ע��
//		{
//			return -1;
//		}
//		audioFrame = av_frame_alloc();
//		audio_pkt_data = pkt.data;
//		audio_pkt_size = pkt.size;
//		while (audio_pkt_size > 0)
//		{
//			if (quit) return -1;
//			int got_picture;
//			memset(audioFrame, 0, sizeof(AVFrame));
//			int ret = avcodec_decode_audio4(aCodecCtx, audioFrame, &got_picture, &pkt);
//			if (ret < 0) {
//				printf("Error in decoding audio frame.\n");
//				exit(0);
//			}
//			//һ֡һ��������ȡ������ nb_samples , channels Ϊ������ 2 ��ʾ 16 λ 2 ���ֽ�
//			data_size = audioFrame->nb_samples * wanted_frame.channels * 2;
//			if (got_picture)
//			{
//				if (swr_ctx != NULL)
//				{
//					swr_free(&swr_ctx);
//					swr_ctx = NULL;
//				}
//				swr_ctx = swr_alloc_set_opts(NULL, wanted_frame.channel_layout,
//					(AVSampleFormat)wanted_frame.format, wanted_frame.sample_rate,
//					audioFrame->channel_layout, (AVSampleFormat)audioFrame->format,
//					audioFrame->sample_rate, 0, NULL);
//				//��ʼ��
//				if (swr_ctx == NULL || swr_init(swr_ctx) < 0)
//				{
//					printf("swr_init error\n");
//					break;
//				}
//				convert_len = swr_convert(swr_ctx, &audio_buf,
//					AVCODEC_MAX_AUDIO_FRAME_SIZE,
//					(const uint8_t**)audioFrame->data,
//					audioFrame->nb_samples);
//			}
//			audio_pkt_size -= ret;
//			if (audioFrame->nb_samples <= 0)
//			{
//				continue;
//			}
//			av_free_packet(&pkt);
//			return data_size;
//		}
//		av_free_packet(&pkt);
//	}
//}
//int find_stream_index(AVFormatContext* pformat_ctx, int* video_stream, int
//	* audio_stream)
//{
//	assert(video_stream != NULL || audio_stream != NULL);
//	int i = 0;
//	int audio_index = -1;
//	int video_index = -1;
//	for (i = 0; i < pformat_ctx->nb_streams; i++)
//	{
//		if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
//		{
//			video_index = i;
//		}
//		if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
//		{
//			audio_index = i;
//		}
//	}
//	//ע�����������ж��п��ܷ���-1.
//	if (video_stream == NULL)
//	{
//		*audio_stream = audio_index;
//		return *audio_stream;
//	}
//	if (audio_stream == NULL)
//	{
//		*video_stream = video_index;
//		return *video_stream;
//	}
//	*video_stream = video_index;
//	*audio_stream = audio_index;
//	return 0;
//}