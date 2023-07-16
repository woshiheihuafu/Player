#include "videoplayer.h"
#define SDL_AUDIO_BUFFER_SIZE 1024 //
#define FLUSH_DATA "FLUSH"//��ձ�־


//��������������ݳ���ĳ����С��ʱ�� ����ͣ��ȡ ��ֹһ���ӾͰ���Ƶ�����ˣ����µĿռ���䲻��
#define MAX_AUDIO_SIZE (1024*16*25*10)//��Ƶ��ֵ
#define MAX_VIDEO_SIZE (1024*255*25*2)//��Ƶ��ֵ

AVFrame wanted_frame;
PacketQueue audio_queue;
int isQuit = 0;
//�ص�����
void audio_callback(void* userdata, Uint8* stream, int len);
//���뺯��
int audio_decode_frame(VideoState* is, uint8_t* audio_buf, int buf_size);
//�� auto_stream
int find_stream_index(AVFormatContext* pformat_ctx, int* video_stream, int* audio_stream);
//ʱ�䲹������--��Ƶ��ʱ
double synchronize_video(VideoState* is, AVFrame* src_frame, double pts);
//�����ص�
int interrupt_callback(void* p);
//��ն���
void packet_queue_flush(PacketQueue* q);
//��Ƶ�����̺߳���
int video_thread(void* arg);
videoplayer::videoplayer(QObject* parent) : QThread(parent)
{
}

videoplayer::~videoplayer()
{
}
void videoplayer::begin()
{
	m_videoState.isQuit = false;
	if (m_playerState != Stop) return;
	m_playerState = Playing;
	this->start();
}
void videoplayer::play()
{
	m_videoState.isPause = false;
	if (m_playerState != Pause) return;
	m_playerState = Playing;
}
void videoplayer::pause()
{
	m_videoState.isPause = true;
	if (m_playerState != Playing) return;
	m_playerState = Pause;
}
//ֹͣ
void videoplayer::stop(bool isWait)
{
	m_videoState.isQuit = true;
	if (isWait) //������־
	{
		while (!m_videoState.readThreadFinished)//�ȴ���ȡ�߳��˳�
		{
			SDL_Delay(10);
		}
	}
	//�ر� SDL ��Ƶ�豸
	if (m_videoState.audioID != 0)
	{
		SDL_LockAudio();
		SDL_PauseAudioDevice(m_videoState.audioID, 1);//ֹͣ����,��ֹͣ��Ƶ�ص�����
		SDL_CloseAudioDevice(m_videoState.audioID);
		SDL_UnlockAudio();
		m_videoState.audioID = 0;
	}
	m_playerState = PlayerState::Stop;
	Q_EMIT SIG_PlayerStateChanged(PlayerState::Stop);
}
//�����ļ���
void videoplayer::setFileName(const QString& fileName)
{
	/*if (m_playerState != PlayerState::Stop) return;*/
	m_fileName = fileName;
	/*m_playerState = PlayerState::Playing;*/
	/*this->start();*/
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


	//��ʼ������
	avformat_network_init();
	avdevice_register_all();
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
	Runner input_runner = { 0 };
	pFmtCtx->interrupt_callback.callback = interrupt_callback;
	pFmtCtx->interrupt_callback.opaque = &input_runner;
	input_runner.lasttime = time(NULL);
	input_runner.connected = false;
	int ret = 0;
	if ((ret = avformat_open_input(&pFmtCtx, m_fileName.toStdString().c_str(), NULL, NULL)) < 0)
	{
		avformat_close_input(&pFmtCtx);
		//������Դ֮��,�������Ӷ�ȡ�ļ��߳��˳���־.
		m_videoState.readThreadFinished = true;
		//��Ƶ�Զ����� �ñ�־λ
		m_playerState = PlayerState::Stop;
		stop(true);
		cout << "3.1 ����Ƶ�ļ�ʧ��!" << endl;
		return;
	}
	else
	{
		input_runner.connected = true;
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
	Q_EMIT SIG_TotalTime(getTotalTime());
	//��ȡ
		//8.ѭ����ȡ
		//int ret, got_picture;
		//int64_t beginTime = av_gettime();
		//int64_t pts = 0;//��ǰ��Ƶ֡��pts
	m_videoState.readFinished = false;
	m_videoState.readThreadFinished = false;
	m_videoState.videoThreadFinished = false;
	while (1)
	{
		if (m_videoState.isQuit) break;
		if (m_videoState.isPause) continue;//�����ͣ��ֹͣ��������ʾ
		//�������˸����� ��������������ݳ���ĳ����С��ʱ�� ����ͣ��ȡ ��ֹһ���ӾͰ���Ƶ�����ˣ����µĿռ���䲻��
			/* ���� audioq.size ��ָ�����е��������ݰ�������Ƶ���ݵ�����������Ƶ���������������ǰ������� */
		   //���ֵ������΢д��һЩ
		if (m_videoState.audioStream != -1 && m_videoState.audioq->size > MAX_AUDIO_SIZE)
		{
			SDL_Delay(10);
			continue;
		}
		if (m_videoState.videoStream != -1 && m_videoState.videoq->size > MAX_VIDEO_SIZE)
		{
			SDL_Delay(10);
			continue;
		}
		//��ת
		if (m_videoState.seek_req)
			// ��ת��־λ seek_req --> 1 ���������Ļ��� 3s --> 3min 3s ��������� ���� ���кͽ�����
			// 3s �ڽ�������������ݺ� 3min �Ļ����һ�� ������ --> ������� �������������AV_flush_...
			//ʲôʱ������ -->Ҫ������ , ����Ҫ����־�� FLUSH_DATA "FLUSH"
			//�ؼ�֡--���� 10 �� --> 15 �� ��ת�ؼ�֡ ֻ���� 10 �� 15 , �����Ҫ���� 13 , ����������10 Ȼ�� 10 - 13 �İ�ȫ�ӵ�
		{
			int stream_index = -1;
			int64_t seek_target = m_videoState.seek_pos;//΢��
			if (m_videoState.videoStream >= 0)
				stream_index = m_videoState.videoStream;
			else if (m_videoState.audioStream >= 0)
				stream_index = m_videoState.audioStream;
			AVRational aVRational = { 1, AV_TIME_BASE };
			if (stream_index >= 0) 
			{
				seek_target = av_rescale_q(seek_target, aVRational,
					pFmtCtx->streams[stream_index]->time_base); //��ת����λ��
			}
			if (av_seek_frame(m_videoState.pFormatCtx, stream_index, seek_target,
				AVSEEK_FLAG_BACKWARD) < 0) 
			{
				fprintf(stderr, "%s: error while seeking\n", m_videoState.pFormatCtx->filename);
			}
			else 
			{
				if (m_videoState.audioStream >= 0) 
				{
					AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket)); //����һ�� packet
					av_new_packet(packet, 10);
					strcpy((char*)packet->data, FLUSH_DATA);
					packet_queue_flush(m_videoState.audioq); //�������
					packet_queue_put(m_videoState.audioq, packet); //�������д�����������İ�
				}
				if (m_videoState.videoStream >= 0) 
				{
					AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket)); //����һ�� packet
					av_new_packet(packet, 10);
					strcpy((char*)packet->data, FLUSH_DATA);
					packet_queue_flush(m_videoState.videoq); //�������
					packet_queue_put(m_videoState.videoq, packet); //�������д�����������İ�
					m_videoState.video_clock = 0; //���ǵ�������� ���⿨��
					//��Ƶ�����������Ƶ ѭ�� SDL_Delay ��ѭ�������� ��Ƶʱ�ӻ�ı� , ���� ��Ƶʱ�ӱ�С
				}
			}
			m_videoState.seek_req = 0;
			m_videoState.seek_time = m_videoState.seek_pos; //��ȷ��΢�� seek_time ����������Ƶ��Ƶ��ʱ�ӵ��� --�ؼ�֡
			m_videoState.seek_flag_audio = 1; //����Ƶ��Ƶѭ���� , �ж�, AVPacket �� FLUSH_DATA��ս���������
			m_videoState.seek_flag_video = 1;
		}
		if (av_read_frame(pFmtCtx, packet) < 0)
		{
			DelayCount++;
			if (DelayCount >= 300)
			{
				m_videoState.readFinished = true;
				DelayCount = 0;
			}
			if (m_videoState.isQuit)break;//������
			SDL_Delay(10);
			continue;
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
		msleep(5);
	}


	//9.��������,��ֹ��Ƶ����Ƶ����δȡ�꣬�ȴ��������ٻ���
	//while (m_videoState.videoStream != -1 && m_videoState.videoq->nb_packets != 0)
	//{
	//	if (m_videoState.isQuit) break;
	//	SDL_Delay(100);
	//}
	//SDL_Delay(100);
	//while (m_videoState.audioStream != -1 && m_videoState.audioq->nb_packets != 0)
	//{
	//	if (m_videoState.isQuit) break;
	//	SDL_Delay(100);
	//}
	//SDL_Delay(100);
	////���տռ�
	//if (audioStream != -1)
	//	avcodec_close(pAudioCodecCtx);
	//if (videoStream != -1)
	//	avcodec_close(pCodecCtx);
	//avformat_close_input(&pFmtCtx);
	//m_videoState.readFinished = true;
	//m_playerState = Stop;
	while (!m_videoState.isQuit)
	{
		SDL_Delay(100);
	}
	//���տռ�
	if (m_videoState.videoStream != -1)
		packet_queue_flush(m_videoState.videoq);//���л���
	if (m_videoState.audioStream != -1)
		packet_queue_flush(m_videoState.audioq); //���л���
	while (m_videoState.videoStream != -1 && !m_videoState.videoThreadFinished)
	{
		SDL_Delay(10);
	}
	if (audioStream != -1)
	{
		//���տռ�
		avcodec_close(pAudioCodecCtx);
	}
	//9.��������
	if (videoStream != -1)
	{
		avcodec_close(pCodecCtx);
	}
	avformat_close_input(&pFmtCtx);
	m_videoState.readThreadFinished = true;
}
void packet_queue_flush(PacketQueue* q)
{
	AVPacketList* pkt, * pkt1;
	SDL_LockMutex(q->mutex);
	for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1)
	{
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	SDL_UnlockMutex(q->mutex);
}

void audio_callback(void* userdata, Uint8* stream, int len)
{
	//AVCodecContext* pcodec_ctx = (AVCodecContext*)userdata;
	VideoState* is = (VideoState*)userdata;
	int len1, audio_data_size;
	memset(stream, 0, len);

	if (is->isPause) return;
	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;
	/* len ���� SDL ����� SDL �������Ĵ�С������������δ�������Ǿ�һֱ����������� */
	/* audio_buf_index �� audio_buf_size ��ʾ�����Լ��������ý�����������ݵĻ�������*/
	/* ��Щ���ݴ� copy �� SDL �������� �� audio_buf_index >= audio_buf_size ��ʱ����ζ����*/
	/* �ǵĻ���Ϊ�գ�û�����ݿɹ� copy����ʱ����Ҫ���� audio_decode_frame ���������
	/* ��������� */
	while (len > 0)
	{
		if (audio_buf_index >= audio_buf_size) {
			audio_data_size = audio_decode_frame(is, audio_buf, sizeof(audio_buf));
			/* audio_data_size < 0 ��ʾû�ܽ�������ݣ�����Ĭ�ϲ��ž��� */
			if (audio_data_size < 0) {
				/* silence */
				audio_buf_size = 1024;
				/* ���㣬���� */
				memset(audio_buf, 0, audio_buf_size);
			}
			else {
				audio_buf_size = audio_data_size;
			}
			audio_buf_index = 0;
		}
		/* �鿴 stream ���ÿռ䣬����һ�� copy �������ݣ�ʣ�µ��´μ��� copy */
		len1 = audio_buf_size - audio_buf_index;
		if (len1 > len) {
			len1 = len;
		}
		//SDL_MixAudio ��������
		//memcpy(stream, (uint8_t*)audio_buf + audio_buf_index, len1);
		memset(stream, 0, len1);
		//�������� sdl 2.0 �汾ʹ�øú��� �滻 SDL_MixAudio
		SDL_MixAudioFormat(stream, (uint8_t*)audio_buf + audio_buf_index, AUDIO_S16SYS, len1, 30);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
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
	//	if (isQuit) return -1;
	//	if (packet_queue_get(audioq, &pkt, 0) <= 0) //һ��ע��
	//	{
	//		return -1;
	//	}
	//	audioFrame = av_frame_alloc();
	//	audio_pkt_data = pkt.data;
	//	audio_pkt_size = pkt.size;
	//	while (audio_pkt_size > 0)
	//	{
	//		if (isQuit) return -1;
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
		if (is->isQuit) return -1;
		if (is->isPause) continue;//�����ͣ���򲻽���
		if (!audioq) return -1;
		if (audioq && packet_queue_get(audioq, &pkt, 0) <= 0) //һ��ע��
		{
			if (is->readFinished && is->audioq->nb_packets == 0)
				is->isQuit = true;
			return -1;
		}
		if (strcmp((char*)pkt.data, FLUSH_DATA) == 0)
		{
			avcodec_flush_buffers(is->audio_st->codec);
			av_free_packet(&pkt);
			continue;
		}
		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
		while (audio_pkt_size > 0)
		{
			if (is->isQuit) return -1;
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
			sampleSize = av_samples_get_buffer_size(NULL, is->pAudioCodecCtx->channels, audioFrame->nb_samples, is->pAudioCodecCtx->sample_fmt, 1);
			//n ��ʾÿ�β������ֽ���
			n = av_get_bytes_per_sample(is->pAudioCodecCtx->sample_fmt) * is->pAudioCodecCtx->channels;
			//ʱ��ÿ��Ҫ��һ֡���ݵ�ʱ��= һ֡���ݵĴ�С/һ���Ӳ��� sample_rate ��ζ�Ӧ���Լ���.
			is->audio_clock += (double)sampleSize * 1000000 / (double)(n * is->pAudioCodecCtx->sample_rate);
			//��ת���ؼ�֡,����һЩ֡
			if (is->seek_flag_audio)
			{
				if (is->audio_clock < is->seek_time) //û�е�Ŀ��ʱ��
				{
					if (pkt.pts != AV_NOPTS_VALUE)
					{
						is->audio_clock = av_q2d(is->audio_st->time_base) * pkt.pts * 1000000; //ȡ��Ƶʱ�� ���ܾ��Ȳ���
					}
					break;
				}
				else
				{
					if (pkt.pts != AV_NOPTS_VALUE)
					{
						is->audio_clock = av_q2d(is->audio_st->time_base) * pkt.pts * 1000000; //ȡ��Ƶʱ�� ���ܾ��Ȳ���
					}
					is->seek_flag_audio = 0;
				}
			}
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
	int64_t begin_pts = 0;
	int64_t begin_pts2 = 0;
	while (1)
	{
		if (is->isQuit) break;
		if (is->isPause) continue;//�����ͣ������������Ƶ
		if (packet_queue_get(is->videoq, packet, 0) <= 0)
		{
			if (is->readFinished && is->audioq->nb_packets == 0)//���ŵ�����
			{//���߳����
				break;
			}
			else
			{
				SDL_Delay(1); //ֻ�Ƕ���������ʱû�����ݶ���
				continue;
			}
		}
		if (strcmp((char*)packet->data, FLUSH_DATA) == 0)
		{
			avcodec_flush_buffers(is->video_st->codec);
			av_free_packet(packet);
			is->video_clock = 0; //�ܹؼ� , ����� ������ת, ��Ƶ֡��ȴ���Ƶ֡
			continue;
		}
		while (1)
		{
			if (is->isQuit) break;
			if (is->audioq->size == 0) break;
			audio_pts = is->audio_clock;
			video_pts = is->video_clock;
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
		if (!is->beginFrame)
		{
			is->beginFrame = true;
			begin_pts = video_pts;

			begin_pts = (begin_pts > 0 ? begin_pts : 0);
		}
		video_pts -= begin_pts;
		if (!is->beginFrame)
		{
			is->beginFrame = true;
			begin_pts2 = audio_pts;

			begin_pts2 = (begin_pts2 > 0 ? begin_pts2 : 0);
		}
		audio_pts -= begin_pts2;
		video_pts *= 1000000 * av_q2d(is->video_st->time_base);
		video_pts = synchronize_video(is, pFrame, video_pts);//��Ƶʱ�Ӳ���
		//����֡��������10�ؼ�֡������ϣ����ת��13��ͨ֡������Ҫ����3֡
		if (is->seek_flag_video)
		{
			//��������ת �������ؼ�֡��Ŀ��ʱ����⼸֡
			if (video_pts < is->seek_time)
			{
				av_free_packet(packet);
				continue;
			}
			else
			{
				is->seek_flag_video = 0;
			}
		}
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
	if (!is->isQuit)
	{
		is->isQuit = true;
	}
	av_free(pFrame);
	av_free(pFrameRGB);
	av_free(out_buffer_rgb);
	is->videoThreadFinished = true;

	// ����
	QImage img; //��ͼ����һ�� ���ݸ�������ʾ
	img.fill(Qt::black);
	is->m_player->SendGetOneImage(img); //���ü����źŵĺ���
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

double videoplayer::getCurrentTime()
{
	return m_videoState.audio_clock;
}
//��ȡ��ʱ��
int64_t videoplayer::getTotalTime()
{
	if (m_videoState.pFormatCtx)
		return m_videoState.pFormatCtx->duration;
	return -1;
}
//��ȡ����״̬
PlayerState videoplayer::getPlayerState() const
{
	return m_playerState;
}
//��ת����
void videoplayer::seek(int64_t pos) //��ȷ��΢��
{
	if (!m_videoState.seek_req)
	{
		m_videoState.seek_pos = pos;
		m_videoState.seek_req = 1;
	}
}


// �ص�����
int interrupt_callback(void* p) {
	Runner* r = (Runner*)p;
	if (r->lasttime > 0) {
		if (time(NULL) - r->lasttime > 5 && !r->connected) {
			// �ȴ����� 5s ���ж�
			return 1;
		}
	}
	return 0;
}
