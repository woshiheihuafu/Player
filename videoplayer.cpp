#include "videoplayer.h"
#define SDL_AUDIO_BUFFER_SIZE 1024 //



//��������������ݳ���ĳ����С��ʱ�� ����ͣ��ȡ ��ֹһ���ӾͰ���Ƶ�����ˣ����µĿռ���䲻��
#define MAX_AUDIO_SIZE (1024*16*25*10)//��Ƶ��ֵ
#define MAX_VIDEO_SIZE (1024*255*25*2)//��Ƶ��ֵ

AVFrame wanted_frame;
PacketQueue audio_queue;
int quit = 0;
//�ص�����
void audio_callback(void* userdata, Uint8* stream, int len);
//���뺯��
int audio_decode_frame(VideoState* is, uint8_t* audio_buf, int buf_size);
//�� auto_stream
int find_stream_index(AVFormatContext* pformat_ctx, int* video_stream, int* audio_stream);
//ʱ�䲹������--��Ƶ��ʱ
double synchronize_video(VideoState* is, AVFrame* src_frame, double pts);

//��Ƶ�����̺߳���
int video_thread(void* arg);
videoplayer::videoplayer(QObject *parent): QThread(parent)
{
}

videoplayer::~videoplayer()
{
}
//�����ļ���
void videoplayer::setFileName(const QString & fileName)
{
	m_fileName = fileName;
}
//�߳�ִ�к��� ��start()����
void videoplayer::run()
{
	//����Ƶ�������
	int audioStream = -1;//��Ƶ��������Ҫ����������
	int videoStream = -1;//��Ƶ��������Ҫ����������
	AVCodecContext* pAudioCodecCtx = NULL;//��Ƶ��������Ϣָ��
	AVCodecContext* pCodecCtx = NULL;//��Ƶ������������
	AVCodec* pAudioCodec = NULL; //��Ƶ������
	AVCodec* pCodec = NULL;//��Ƶ������
	AVFrame* pFrame;//YUV
	AVFrame* pFrameRGB;//RGB
	int numBytes;//֡���ݴ�С
	uint8_t* out_buffer;//�洢ת��Ϊ RGB ��ʽ���ݵĻ�����
	static struct SwsContext* img_convert_ctx;//YUV ת RGB �Ľṹ
	//SDL
	SDL_AudioSpec wanted_spec; //SDL ��Ƶ����
	SDL_AudioSpec spec; //SDL ��Ƶ����



	//0.videoState��ʼ��
	memset(&m_videoState, 0, sizeof(m_videoState));

	//1.��ʼ��ffmpeg��SDL
	av_register_all();
	//1.1SDL ��ʼ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		cout << "1.1��ʼ��SDLʧ��!" << endl;
		cout << "Couldn't init SDL: " << SDL_GetError() << endl;
		return;
	}
	cout << "1.��ʼ��ffmpeg��SDL�ɹ�!" << endl;
	//2.����AVFormatContext������
	AVFormatContext* pFmtCtx = avformat_alloc_context();
	cout << "2.����AVFormatContext�����ĳɹ�!" << endl;
	//3.1����Ƶ�ļ�
	if (avformat_open_input(&pFmtCtx, m_fileName.toStdString().c_str(), NULL, NULL) < 0)
	{
		cout << "3.1 ����Ƶ�ļ�ʧ��!" << endl;
		return;
	}
	cout << "3.1 ����Ƶ�ļ��ɹ�!" << endl;
	//3.2��ȡ��Ƶ�ļ���Ϣ
	if (avformat_find_stream_info(pFmtCtx, NULL) < 0)
	{
		cout << "3.2��ȡ��Ƶ��Ϣʧ�ܣ�" << endl;
		return;
	}
	cout << "3.2 ��ȡ��Ƶ��Ϣ�ɹ�!" << endl;
	av_dump_format(pFmtCtx, 0, m_fileName.toStdString().c_str(), false);
	//4.������Ƶ��Ƶ������
	if (find_stream_index(pFmtCtx, &videoStream, &audioStream) == -1)
	{
		cout << "Couldn't find stream index" << endl;
		cout << "4.û���ҵ���Ƶ��!" << endl;
		return;
	}
	cout << "4.��ȡ��Ƶ���ɹ�!" << endl;
//��Ƶ����
	m_videoState.pFormatCtx = pFmtCtx;
	m_videoState.videoStream = videoStream;
	m_videoState.audioStream = audioStream;
	m_videoState.m_player = this;
	//5.���Ҳ��򿪽�����
	if (videoStream != -1)
	{
		pCodecCtx = pFmtCtx->streams[videoStream]->codec;
		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL)
		{
			cout << "Video 5.���ҽ�����ʧ��!" << endl;
			return;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			cout << "Video 5.�򿪽�����ʧ�ܣ�" << endl;
			return;
		}
		cout << "Video 5.1 �򿪽������ɹ�!" << endl;
		////6.���������Ҫ�Ľṹ��
		//pFrame = av_frame_alloc();
		//pFrameRGB = av_frame_alloc();
		//int y_size = pCodecCtx->width * pCodecCtx->height;
		////AVPacket packet;
		//av_new_packet(packet, y_size);
		//cout << "Video 6.���������Ҫ�Ľṹ��ɹ�!" << endl;
		////7.YUV����ת��ΪRGB32����Ҫ�Ĵ洢
		//img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		//	pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
		//	AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
		//numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
		//out_buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
		//avpicture_fill((AVPicture*)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
		//cout << "Video 7.YUV����ת��ΪRGB32����Ҫ�Ĵ洢�ɹ�!" << endl;
		//��Ƶ��
		m_videoState.video_st = pFmtCtx->streams[videoStream];
		m_videoState.pCodecCtx = pCodecCtx;
		//��Ƶͬ������
		m_videoState.videoq = new PacketQueue;
		packet_queue_init(m_videoState.videoq);
		//������Ƶ�߳�
		m_videoState.video_tid = SDL_CreateThread(video_thread, "video_thread", &m_videoState);
	}
//��Ƶ����
	if (audioStream != -1)
	{
		//5.�ҵ���Ӧ����Ƶ������
		pAudioCodecCtx = pFmtCtx->streams[audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
		if (!pAudioCodec)
		{
			cout << "Audio 5. ���ҽ�����ʧ��!" << endl;
			cout << "Couldn't find decoder" << endl;
			return;
		}
		cout << "Audio 5.���ҽ������ɹ�!" << endl;
		//����Ƶ������
		avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL);
		cout << "Audio 5.1�򿪽������ɹ�!" << endl;
		m_videoState.audio_st = pFmtCtx->streams[audioStream];
		m_videoState.pAudioCodecCtx = pAudioCodecCtx;
		//6.������Ƶ��Ϣ, ��������Ƶ�豸��
		SDL_LockAudio();
		wanted_spec.freq = pAudioCodecCtx->sample_rate;
		switch (pFmtCtx->streams[audioStream]->codec->sample_fmt)
		{
		case AV_SAMPLE_FMT_U8:
			wanted_spec.format = AUDIO_S8;
			break;
		case AV_SAMPLE_FMT_S16:
			wanted_spec.format = AUDIO_S16SYS;
			break;
		default:
			wanted_spec.format = AUDIO_S16SYS;
			break;
		};
		wanted_spec.channels = pAudioCodecCtx->channels; //ͨ����
		wanted_spec.silence = 0; //���þ���ֵ
		wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE; //��ȡ��һ֡�����
		wanted_spec.callback = audio_callback;//�ص�����
		wanted_spec.userdata = /*pAudioCodecCtx*/&m_videoState;//�ص���������
		m_videoState.audioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), 0, &wanted_spec, &spec, 0);
		if (m_videoState.audioID < 0) //�ڶ��δ� audio �᷵��-1
		{
			printf("Couldn't open Audio: %s\n", SDL_GetError());
			return;
		}
		//���ò�����������ʱ����, swr_alloc_set_opts �� in ���ֲ���
		switch (pFmtCtx->streams[audioStream]->codec->sample_fmt)
		{
		case AV_SAMPLE_FMT_U8:
			m_videoState.out_frame.format = AV_SAMPLE_FMT_U8;
			break;
		case AV_SAMPLE_FMT_S16:
			m_videoState.out_frame.format = AV_SAMPLE_FMT_S16;
			break;
		default:
			m_videoState.out_frame.format = AV_SAMPLE_FMT_S16;
			break;
		};
		m_videoState.out_frame.sample_rate = pAudioCodecCtx->sample_rate;
		m_videoState.out_frame.channel_layout = av_get_default_channel_layout(pAudioCodecCtx->channels);
		m_videoState.out_frame.channels = pAudioCodecCtx->channels;
		m_videoState.audioq = new PacketQueue;
		//��ʼ������
		packet_queue_init(m_videoState.audioq);
		SDL_UnlockAudio();
		// SDL �������� 0 ����
		SDL_PauseAudioDevice(m_videoState.audioID, 0);
		////6.������Ƶ��Ϣ, ��������Ƶ�豸��
		//wanted_spec.freq = pAudioCodecCtx->sample_rate;
		//wanted_spec.format = AUDIO_S16SYS;
		//wanted_spec.channels = pAudioCodecCtx->channels; //ͨ����
		//wanted_spec.silence = 0; //���þ���ֵ
		//wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE; //��ȡ��һ֡�����
		//wanted_spec.callback = audio_callback;//�ص�����
		//wanted_spec.userdata = pAudioCodecCtx;//�ص���������
		//cout << "Audio 6.��Ƶ��Ϣ���óɹ�!" << endl;
		////7.����Ƶ�豸
		//SDL_AudioDeviceID id =
		//	SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), 0, &wanted_spec, &spec, 0);
		//if (id < 0) //�ڶ��δ� audio �᷵��-1
		//{
		//	cout << "Audio 7.����Ƶ�豸ʧ��!" << endl;
		//	cout << "Couldn't open Audio: " << SDL_GetError();
		//	return;
		//}
		//cout << "Audio 7.����Ƶ�豸�ɹ�!" << endl;
		//8.���ò�����������ʱ����, swr_alloc_set_opts �� in ���ֲ���
		//wanted_frame.format = AV_SAMPLE_FMT_S16;
		//wanted_frame.sample_rate = spec.freq;
		//wanted_frame.channel_layout = av_get_default_channel_layout(spec.channels);
		//wanted_frame.channels = spec.channels;
		//cout << "Audio 8.���ý�������ɹ�!" << endl;
		////9.��ʼ������
		//packet_queue_init(&audio_queue);
		//cout << "Audio 9.��ʼ�����гɹ�!" << endl;
		////10.������Ƶ
		//// SDL �������� 0 ����
		//SDL_PauseAudioDevice(id,0);
		//cout << "Audio 10.��ʼ������Ƶ!" << endl;
	}
	AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket)); //����һ�� packet
//��ȡ
	//8.ѭ����ȡ
	//int ret, got_picture;
	//int64_t beginTime = av_gettime();
	//int64_t pts = 0;//��ǰ��Ƶ֡��pts
	while (1)
	{
		if (m_videoState.quit) break;
		//�������˸����� ��������������ݳ���ĳ����С��ʱ�� ����ͣ��ȡ ��ֹһ���ӾͰ���Ƶ�����ˣ����µĿռ���䲻��
			/* ���� audioq.size ��ָ�����е��������ݰ�������Ƶ���ݵ�����������Ƶ���������������ǰ������� */
		   //���ֵ������΢д��һЩ
		if (m_videoState.audioStream != -1 && m_videoState.audioq->size >MAX_AUDIO_SIZE) 
		{
			SDL_Delay(10);
			continue;
		}
		if (m_videoState.videoStream != -1 && m_videoState.videoq->size >MAX_VIDEO_SIZE) 
		{
			SDL_Delay(10);
			continue;
		}

		if (av_read_frame(pFmtCtx, packet) < 0)
		{
			break;//������
		}
		//����ͼƬ
		if (packet->stream_index == videoStream)
		{
			
			packet_queue_put(m_videoState.videoq, packet);
		}
		else if (packet->stream_index == audioStream)
		{
			packet_queue_put(m_videoState.audioq, packet);
		}
		else
		{
			av_free_packet(packet);
		}
		//msleep(5);
	}


	//9.��������,��ֹ��Ƶ����Ƶ����δȡ�꣬�ȴ��������ٻ���
	while (m_videoState.videoStream != -1 && m_videoState.videoq->nb_packets != 0)
	{
		if (m_videoState.quit) break;
		SDL_Delay(100);
	}
	SDL_Delay(100);
	while (m_videoState.audioStream != -1 && m_videoState.audioq->nb_packets != 0)
	{
		if (m_videoState.quit) break;
		SDL_Delay(100);
	}
	SDL_Delay(100);
	//���տռ�
	if (audioStream != -1)
		avcodec_close(pAudioCodecCtx);
	if (videoStream != -1)
		avcodec_close(pCodecCtx);
	avformat_close_input(&pFmtCtx);

}


void audio_callback(void* userdata, Uint8* stream, int len)
{
	//AVCodecContext* pcodec_ctx = (AVCodecContext*)userdata;
	VideoState* is = (VideoState*)userdata;
	int len1, audio_data_size;
	/*static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;*/
	/* len ���� SDL ����� SDL �������Ĵ�С������������δ�������Ǿ�һֱ����������� */
	/* audio_buf_index �� audio_buf_size ��ʾ�����Լ��������ý�����������ݵĻ�������*/
	/* ��Щ���ݴ� copy �� SDL �������� �� audio_buf_index >= audio_buf_size ��ʱ����ζ����*/
	/* �ǵĻ���Ϊ�գ�û�����ݿɹ� copy����ʱ����Ҫ���� audio_decode_frame ���������
	/* ��������� */
	while (len > 0)
	{
		if (is->audio_buf_index >= is->audio_buf_size) {
			audio_data_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf));
			/* audio_data_size < 0 ��ʾû�ܽ�������ݣ�����Ĭ�ϲ��ž��� */
			if (audio_data_size < 0) {
				/* silence */
				is->audio_buf_size = 1024;
				/* ���㣬���� */
				memset(is->audio_buf, 0, is->audio_buf_size);
			}
			else {
				is->audio_buf_size = audio_data_size;
			}
			is->audio_buf_index = 0;
		}
		/* �鿴 stream ���ÿռ䣬����һ�� copy �������ݣ�ʣ�µ��´μ��� copy */
		len1 = is->audio_buf_size - is->audio_buf_index;
		if (len1 > len) {
			len1 = len;
		}
		//SDL_MixAudio ��������
		//memcpy(stream, (uint8_t*)audio_buf + audio_buf_index, len1);
		memset(stream, 0, len1);
		//�������� sdl 2.0 �汾ʹ�øú��� �滻 SDL_MixAudio
		SDL_MixAudioFormat(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 30);
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
	}
}
//������Ƶ��˵��һ�� packet ���棬���ܺ��ж�֡(frame)���ݡ�
int audio_decode_frame(VideoState* is, uint8_t* audio_buf, int buf_size)
{
	//static AVPacket pkt;
	//static uint8_t* audio_pkt_data = NULL;
	//static int audio_pkt_size = 0;
	///*AVPacket pkt;
	//uint8_t* audio_pkt_data = NULL;
	//int audio_pkt_size = 0;*/
	//int len1, data_size;
	//int sampleSize = 0;
	//AVPacket pktx;
	//AVCodecContext* aCodecCtx = pcodec_ctx;
	//AVFrame* audioFrame = NULL;
	//PacketQueue* audioq = &audio_queue;
	//static struct SwrContext* swr_ctx = NULL;
	//int convert_len;
	//int n = 0;
	//for (;;)
	//{
	//	if (quit) return -1;
	//	if (packet_queue_get(audioq, &pkt, 0) <= 0) //һ��ע��
	//	{
	//		return -1;
	//	}
	//	audioFrame = av_frame_alloc();
	//	audio_pkt_data = pkt.data;
	//	audio_pkt_size = pkt.size;
	//	while (audio_pkt_size > 0)
	//	{
	//		if (quit) return -1;
	//		int got_picture;
	//		memset(audioFrame, 0, sizeof(AVFrame));
	//		int ret = avcodec_decode_audio4(aCodecCtx, audioFrame, &got_picture, &pkt);
	//		if (ret < 0) {
	//			printf("Error in decoding audio frame.\n");
	//			exit(0);
	//		}
	//		//һ֡һ��������ȡ������ nb_samples , channels Ϊ������ 2 ��ʾ 16 λ 2 ���ֽ�
	//		data_size = audioFrame->nb_samples * wanted_frame.channels * 2;
	//		if (got_picture)
	//		{
	//			if (swr_ctx != NULL)
	//			{
	//				swr_free(&swr_ctx);
	//				swr_ctx = NULL;
	//			}
	//			swr_ctx = swr_alloc_set_opts(NULL, wanted_frame.channel_layout,
	//				(AVSampleFormat)wanted_frame.format, wanted_frame.sample_rate,
	//				audioFrame->channel_layout, (AVSampleFormat)audioFrame->format,
	//				audioFrame->sample_rate, 0, NULL);
	//			//��ʼ��
	//			if (swr_ctx == NULL || swr_init(swr_ctx) < 0)
	//			{
	//				printf("swr_init error\n");
	//				break;
	//			}
	//			convert_len = swr_convert(swr_ctx, &audio_buf,
	//				AVCODEC_MAX_AUDIO_FRAME_SIZE,
	//				(const uint8_t**)audioFrame->data,
	//				audioFrame->nb_samples);
	//		}
	//		audio_pkt_size -= ret;
	//		if (audioFrame->nb_samples <= 0)
	//		{
	//			continue;
	//		}
	//		av_free_packet(&pkt);
	//		return data_size;
	//	}
	//	av_free_packet(&pkt);
	//}
	AVPacket pkt;
	uint8_t* audio_pkt_data = NULL;
	int audio_pkt_size = 0;
	int len1, data_size;
	int sampleSize = 0;
	AVCodecContext* aCodecCtx = is->pAudioCodecCtx;
	AVFrame* audioFrame = av_frame_alloc();
	PacketQueue* audioq = is->audioq;
	AVFrame wanted_frame = is->out_frame;
	if (!aCodecCtx || !audioFrame || !audioq) return -1;
	/*static */struct SwrContext* swr_ctx = NULL;
	int convert_len;
	int n = 0;
	for (;;)
	{
		if (is->quit) return -1;
		if (!audioq) return -1;
		if (packet_queue_get(audioq, &pkt, 0) <= 0) //һ��ע��
		{
			return -1;
		}
		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
		while (audio_pkt_size > 0)
		{
			if (is->quit) return -1;
			int got_picture;
			memset(audioFrame, 0, sizeof(AVFrame));
			int ret = avcodec_decode_audio4(aCodecCtx, audioFrame, &got_picture, &pkt);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				exit(0);
			}
			//һ֡һ��������ȡ�����ֽ����� nb_samples , channels Ϊ������ 2 ��ʾ 16 λ 2 ���ֽ�
			 //data_size = audioFrame->nb_samples * wanted_frame.channels * 2;
			switch (is->out_frame.format)
			{
			case AV_SAMPLE_FMT_U8:
				data_size = audioFrame->nb_samples * is->out_frame.channels * 1;
				break;
			case AV_SAMPLE_FMT_S16:
				data_size = audioFrame->nb_samples * is->out_frame.channels * 2;
				break;
			default:
				data_size = audioFrame->nb_samples * is->out_frame.channels * 2;
				break;
			}
			//sampleSize ��ʾһ֡(��С nb_samples)audioFrame ��Ƶ���ݶ�Ӧ���ֽ���.
			sampleSize = av_samples_get_buffer_size(NULL, is->pAudioCodecCtx->channels,audioFrame->nb_samples,is->pAudioCodecCtx->sample_fmt, 1);
			//n ��ʾÿ�β������ֽ���
			n = av_get_bytes_per_sample(is->pAudioCodecCtx->sample_fmt) * is->pAudioCodecCtx->channels;
			//ʱ��ÿ��Ҫ��һ֡���ݵ�ʱ��= һ֡���ݵĴ�С/һ���Ӳ��� sample_rate ��ζ�Ӧ���Լ���.
			is->audio_clock += (double)sampleSize * 1000000 / (double)(n * is->pAudioCodecCtx->sample_rate);
			if (got_picture)
			{
				swr_ctx = swr_alloc_set_opts(NULL, wanted_frame.channel_layout,
					(AVSampleFormat)wanted_frame.format, wanted_frame.sample_rate,
					audioFrame->channel_layout, (AVSampleFormat)audioFrame->format,
					audioFrame->sample_rate, 0, NULL);
				//��ʼ��
				if (swr_ctx == NULL || swr_init(swr_ctx) < 0)
				{
					printf("swr_init error\n");
					break;
				}
				convert_len = swr_convert(swr_ctx, &audio_buf,
					AVCODEC_MAX_AUDIO_FRAME_SIZE,
					(const uint8_t**)audioFrame->data,
					audioFrame->nb_samples);
			}
			audio_pkt_size -= ret;
			if (audioFrame->nb_samples <= 0)
			{
				continue;
			}
			av_free_packet(&pkt);
			return data_size;
		}
		av_free_packet(&pkt);
	}
}
int find_stream_index(AVFormatContext* pformat_ctx, int* video_stream, int* audio_stream)
{
	assert(video_stream != NULL || audio_stream != NULL);
	int i = 0;
	int audio_index = -1;
	int video_index = -1;
	for (i = 0; i < pformat_ctx->nb_streams; i++)
	{
		if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
		}
		if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_index = i;
		}
	}
	//ע�����������ж��п��ܷ���-1.
	if (video_stream == NULL)
	{
		*audio_stream = audio_index;
		return *audio_stream;
	}
	if (audio_stream == NULL)
	{
		*video_stream = video_index;
		return *video_stream;
	}
	*video_stream = video_index;
	*audio_stream = audio_index;
	return 0;

}

//��Ƶ�����̺߳���
int video_thread(void* arg)
{
	VideoState* is = (VideoState*)arg;
	AVPacket pkt1, * packet = &pkt1;
	int ret, got_picture, numBytes;
	double video_pts = 0; //��ǰ��Ƶ�� pts
	double audio_pts = 0; //��Ƶ pts
	///������Ƶ���
	AVFrame* pFrame, * pFrameRGB;
	uint8_t* out_buffer_rgb; //������ rgb ����
	struct SwsContext* img_convert_ctx; //���ڽ�������Ƶ��ʽת��
	AVCodecContext* pCodecCtx = is->pCodecCtx; //��Ƶ������
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();
	///�������Ǹĳ��� �������� YUV ����ת���� RGB32
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
		AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
	numBytes = avpicture_get_size(AV_PIX_FMT_RGB32,
		pCodecCtx->width, pCodecCtx->height);
	out_buffer_rgb = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill((AVPicture*)pFrameRGB, out_buffer_rgb, AV_PIX_FMT_RGB32,
		pCodecCtx->width, pCodecCtx->height);
	while (1)
	{
		if (packet_queue_get(is->videoq, packet, 1) <= 0) break;//��������û�������� ��ȡ�����
			while (1)
			{
				audio_pts = is->audio_clock;
				if (video_pts <= audio_pts) break;
				SDL_Delay(5);
			}
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)
			pFrame->opaque != AV_NOPTS_VALUE)
		{
			video_pts = *(uint64_t*)pFrame->opaque;
		}
		else if (packet->dts != AV_NOPTS_VALUE)
		{
			video_pts = packet->dts;
		}
		else
		{
			video_pts = 0;
		}
		video_pts *= 1000000 * av_q2d(is->video_st->time_base);
		video_pts = synchronize_video(is, pFrame, video_pts);//��Ƶʱ�Ӳ���
		if (got_picture) {
			sws_scale(img_convert_ctx,
				(uint8_t const* const*)pFrame->data,
				pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
				pFrameRGB->linesize);
			//����� RGB ���� �� QImage ����
			QImage tmpImg((uchar
				*)out_buffer_rgb, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
			QImage image = tmpImg.copy(); //��ͼ����һ�� ���ݸ�������ʾ
			is->m_player->SendGetOneImage(image); //���ü����źŵĺ���
		}
		av_free_packet(packet);
	}
	av_free(pFrame);
	av_free(pFrameRGB);
	av_free(out_buffer_rgb);
	return 0;
}

void videoplayer::SendGetOneImage(QImage& img)
{
	Q_EMIT SIG_getOneImage(img); //�����ź�
}
double synchronize_video(VideoState* is, AVFrame* src_frame, double pts) {
	double frame_delay;
	if (pts != 0) {
		/* if we have pts, set video clock to it */
		is->video_clock = pts;
	}
	else {
		/* if we aren't given a pts, set it to the clock */
		pts = is->video_clock;
	}
	/* update the video clock */
	frame_delay = av_q2d(is->video_st->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	is->video_clock += frame_delay;
	return pts;
}