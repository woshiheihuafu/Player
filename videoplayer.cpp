#include "videoplayer.h"
#define SDL_AUDIO_BUFFER_SIZE 1024 //



//当队列里面的数据超过某个大小的时候 就暂停读取 防止一下子就把视频读完了，导致的空间分配不足
#define MAX_AUDIO_SIZE (1024*16*25*10)//音频阈值
#define MAX_VIDEO_SIZE (1024*255*25*2)//视频阈值

AVFrame wanted_frame;
PacketQueue audio_queue;
int quit = 0;
//回调函数
void audio_callback(void* userdata, Uint8* stream, int len);
//解码函数
int audio_decode_frame(VideoState* is, uint8_t* audio_buf, int buf_size);
//找 auto_stream
int find_stream_index(AVFormatContext* pformat_ctx, int* video_stream, int* audio_stream);
//时间补偿函数--视频延时
double synchronize_video(VideoState* is, AVFrame* src_frame, double pts);

//视频解码线程函数
int video_thread(void* arg);
videoplayer::videoplayer(QObject *parent): QThread(parent)
{
}

videoplayer::~videoplayer()
{
}
//设置文件名
void videoplayer::setFileName(const QString & fileName)
{
	m_fileName = fileName;
}
//线程执行函数 用start()开启
void videoplayer::run()
{
	//①音频所需变量
	int audioStream = -1;//音频解码器需要的流的索引
	int videoStream = -1;//视频解码器需要的流的索引
	AVCodecContext* pAudioCodecCtx = NULL;//音频解码器信息指针
	AVCodecContext* pCodecCtx = NULL;//视频解码器上下文
	AVCodec* pAudioCodec = NULL; //音频解码器
	AVCodec* pCodec = NULL;//视频解码器
	AVFrame* pFrame;//YUV
	AVFrame* pFrameRGB;//RGB
	int numBytes;//帧数据大小
	uint8_t* out_buffer;//存储转化为 RGB 格式数据的缓冲区
	static struct SwsContext* img_convert_ctx;//YUV 转 RGB 的结构
	//SDL
	SDL_AudioSpec wanted_spec; //SDL 音频设置
	SDL_AudioSpec spec; //SDL 音频设置



	//0.videoState初始化
	memset(&m_videoState, 0, sizeof(m_videoState));

	//1.初始化ffmpeg和SDL
	av_register_all();
	//1.1SDL 初始化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		cout << "1.1初始化SDL失败!" << endl;
		cout << "Couldn't init SDL: " << SDL_GetError() << endl;
		return;
	}
	cout << "1.初始化ffmpeg和SDL成功!" << endl;
	//2.分配AVFormatContext上下文
	AVFormatContext* pFmtCtx = avformat_alloc_context();
	cout << "2.分配AVFormatContext上下文成功!" << endl;
	//3.1打开视频文件
	if (avformat_open_input(&pFmtCtx, m_fileName.toStdString().c_str(), NULL, NULL) < 0)
	{
		cout << "3.1 打开视频文件失败!" << endl;
		return;
	}
	cout << "3.1 打开视频文件成功!" << endl;
	//3.2获取视频文件信息
	if (avformat_find_stream_info(pFmtCtx, NULL) < 0)
	{
		cout << "3.2获取视频信息失败！" << endl;
		return;
	}
	cout << "3.2 获取视频信息成功!" << endl;
	av_dump_format(pFmtCtx, 0, m_fileName.toStdString().c_str(), false);
	//4.查找音频视频流索引
	if (find_stream_index(pFmtCtx, &videoStream, &audioStream) == -1)
	{
		cout << "Couldn't find stream index" << endl;
		cout << "4.没有找到视频流!" << endl;
		return;
	}
	cout << "4.读取视频流成功!" << endl;
//视频部分
	m_videoState.pFormatCtx = pFmtCtx;
	m_videoState.videoStream = videoStream;
	m_videoState.audioStream = audioStream;
	m_videoState.m_player = this;
	//5.查找并打开解码器
	if (videoStream != -1)
	{
		pCodecCtx = pFmtCtx->streams[videoStream]->codec;
		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL)
		{
			cout << "Video 5.查找解码器失败!" << endl;
			return;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			cout << "Video 5.打开解码器失败！" << endl;
			return;
		}
		cout << "Video 5.1 打开解码器成功!" << endl;
		////6.申请解码需要的结构体
		//pFrame = av_frame_alloc();
		//pFrameRGB = av_frame_alloc();
		//int y_size = pCodecCtx->width * pCodecCtx->height;
		////AVPacket packet;
		//av_new_packet(packet, y_size);
		//cout << "Video 6.申请解码需要的结构体成功!" << endl;
		////7.YUV数据转换为RGB32所需要的存储
		//img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		//	pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
		//	AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
		//numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
		//out_buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
		//avpicture_fill((AVPicture*)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
		//cout << "Video 7.YUV数据转换为RGB32所需要的存储成功!" << endl;
		//视频流
		m_videoState.video_st = pFmtCtx->streams[videoStream];
		m_videoState.pCodecCtx = pCodecCtx;
		//视频同步队列
		m_videoState.videoq = new PacketQueue;
		packet_queue_init(m_videoState.videoq);
		//创建视频线程
		m_videoState.video_tid = SDL_CreateThread(video_thread, "video_thread", &m_videoState);
	}
//音频部分
	if (audioStream != -1)
	{
		//5.找到对应的音频解码器
		pAudioCodecCtx = pFmtCtx->streams[audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
		if (!pAudioCodec)
		{
			cout << "Audio 5. 查找解码器失败!" << endl;
			cout << "Couldn't find decoder" << endl;
			return;
		}
		cout << "Audio 5.查找解码器成功!" << endl;
		//打开音频解码器
		avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL);
		cout << "Audio 5.1打开解码器成功!" << endl;
		m_videoState.audio_st = pFmtCtx->streams[audioStream];
		m_videoState.pAudioCodecCtx = pAudioCodecCtx;
		//6.设置音频信息, 用来打开音频设备。
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
		wanted_spec.channels = pAudioCodecCtx->channels; //通道数
		wanted_spec.silence = 0; //设置静音值
		wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE; //读取第一帧后调整
		wanted_spec.callback = audio_callback;//回调函数
		wanted_spec.userdata = /*pAudioCodecCtx*/&m_videoState;//回调函数参数
		m_videoState.audioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), 0, &wanted_spec, &spec, 0);
		if (m_videoState.audioID < 0) //第二次打开 audio 会返回-1
		{
			printf("Couldn't open Audio: %s\n", SDL_GetError());
			return;
		}
		//设置参数，供解码时候用, swr_alloc_set_opts 的 in 部分参数
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
		//初始化队列
		packet_queue_init(m_videoState.audioq);
		SDL_UnlockAudio();
		// SDL 播放声音 0 播放
		SDL_PauseAudioDevice(m_videoState.audioID, 0);
		////6.设置音频信息, 用来打开音频设备。
		//wanted_spec.freq = pAudioCodecCtx->sample_rate;
		//wanted_spec.format = AUDIO_S16SYS;
		//wanted_spec.channels = pAudioCodecCtx->channels; //通道数
		//wanted_spec.silence = 0; //设置静音值
		//wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE; //读取第一帧后调整
		//wanted_spec.callback = audio_callback;//回调函数
		//wanted_spec.userdata = pAudioCodecCtx;//回调函数参数
		//cout << "Audio 6.音频信息设置成功!" << endl;
		////7.打开音频设备
		//SDL_AudioDeviceID id =
		//	SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), 0, &wanted_spec, &spec, 0);
		//if (id < 0) //第二次打开 audio 会返回-1
		//{
		//	cout << "Audio 7.打开音频设备失败!" << endl;
		//	cout << "Couldn't open Audio: " << SDL_GetError();
		//	return;
		//}
		//cout << "Audio 7.打开音频设备成功!" << endl;
		//8.设置参数，供解码时候用, swr_alloc_set_opts 的 in 部分参数
		//wanted_frame.format = AV_SAMPLE_FMT_S16;
		//wanted_frame.sample_rate = spec.freq;
		//wanted_frame.channel_layout = av_get_default_channel_layout(spec.channels);
		//wanted_frame.channels = spec.channels;
		//cout << "Audio 8.设置解码参数成功!" << endl;
		////9.初始化队列
		//packet_queue_init(&audio_queue);
		//cout << "Audio 9.初始化队列成功!" << endl;
		////10.播放音频
		//// SDL 播放声音 0 播放
		//SDL_PauseAudioDevice(id,0);
		//cout << "Audio 10.开始播放音频!" << endl;
	}
	AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket)); //分配一个 packet
//读取
	//8.循环读取
	//int ret, got_picture;
	//int64_t beginTime = av_gettime();
	//int64_t pts = 0;//当前视频帧的pts
	while (1)
	{
		if (m_videoState.quit) break;
		//这里做了个限制 当队列里面的数据超过某个大小的时候 就暂停读取 防止一下子就把视频读完了，导致的空间分配不足
			/* 这里 audioq.size 是指队列中的所有数据包带的音频数据的总量或者视频数据总量，并不是包的数量 */
		   //这个值可以稍微写大一些
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
			break;//读完了
		}
		//生成图片
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


	//9.回收数据,防止视频或音频队列未取完，等待播放完再回收
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
	//回收空间
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
	/* len 是由 SDL 传入的 SDL 缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
	/* audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
	/* 这些数据待 copy 到 SDL 缓冲区， 当 audio_buf_index >= audio_buf_size 的时候意味着我*/
	/* 们的缓冲为空，没有数据可供 copy，这时候需要调用 audio_decode_frame 来解码出更
	/* 多的桢数据 */
	while (len > 0)
	{
		if (is->audio_buf_index >= is->audio_buf_size) {
			audio_data_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf));
			/* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
			if (audio_data_size < 0) {
				/* silence */
				is->audio_buf_size = 1024;
				/* 清零，静音 */
				memset(is->audio_buf, 0, is->audio_buf_size);
			}
			else {
				is->audio_buf_size = audio_data_size;
			}
			is->audio_buf_index = 0;
		}
		/* 查看 stream 可用空间，决定一次 copy 多少数据，剩下的下次继续 copy */
		len1 = is->audio_buf_size - is->audio_buf_index;
		if (len1 > len) {
			len1 = len;
		}
		//SDL_MixAudio 并不能用
		//memcpy(stream, (uint8_t*)audio_buf + audio_buf_index, len1);
		memset(stream, 0, len1);
		//混音函数 sdl 2.0 版本使用该函数 替换 SDL_MixAudio
		SDL_MixAudioFormat(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 30);
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
	}
}
//对于音频来说，一个 packet 里面，可能含有多帧(frame)数据。
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
	//	if (packet_queue_get(audioq, &pkt, 0) <= 0) //一定注意
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
	//		//一帧一个声道读取数据是 nb_samples , channels 为声道数 2 表示 16 位 2 个字节
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
	//			//初始化
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
		if (packet_queue_get(audioq, &pkt, 0) <= 0) //一定注意
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
			//一帧一个声道读取数据字节数是 nb_samples , channels 为声道数 2 表示 16 位 2 个字节
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
			//sampleSize 表示一帧(大小 nb_samples)audioFrame 音频数据对应的字节数.
			sampleSize = av_samples_get_buffer_size(NULL, is->pAudioCodecCtx->channels,audioFrame->nb_samples,is->pAudioCodecCtx->sample_fmt, 1);
			//n 表示每次采样的字节数
			n = av_get_bytes_per_sample(is->pAudioCodecCtx->sample_fmt) * is->pAudioCodecCtx->channels;
			//时钟每次要加一帧数据的时间= 一帧数据的大小/一秒钟采样 sample_rate 多次对应的自己数.
			is->audio_clock += (double)sampleSize * 1000000 / (double)(n * is->pAudioCodecCtx->sample_rate);
			if (got_picture)
			{
				swr_ctx = swr_alloc_set_opts(NULL, wanted_frame.channel_layout,
					(AVSampleFormat)wanted_frame.format, wanted_frame.sample_rate,
					audioFrame->channel_layout, (AVSampleFormat)audioFrame->format,
					audioFrame->sample_rate, 0, NULL);
				//初始化
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
	//注意以下两个判断有可能返回-1.
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

//视频解码线程函数
int video_thread(void* arg)
{
	VideoState* is = (VideoState*)arg;
	AVPacket pkt1, * packet = &pkt1;
	int ret, got_picture, numBytes;
	double video_pts = 0; //当前视频的 pts
	double audio_pts = 0; //音频 pts
	///解码视频相关
	AVFrame* pFrame, * pFrameRGB;
	uint8_t* out_buffer_rgb; //解码后的 rgb 数据
	struct SwsContext* img_convert_ctx; //用于解码后的视频格式转换
	AVCodecContext* pCodecCtx = is->pCodecCtx; //视频解码器
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();
	///这里我们改成了 将解码后的 YUV 数据转换成 RGB32
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
		if (packet_queue_get(is->videoq, packet, 1) <= 0) break;//队列里面没有数据了 读取完毕了
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
		video_pts = synchronize_video(is, pFrame, video_pts);//视频时钟补偿
		if (got_picture) {
			sws_scale(img_convert_ctx,
				(uint8_t const* const*)pFrame->data,
				pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
				pFrameRGB->linesize);
			//把这个 RGB 数据 用 QImage 加载
			QImage tmpImg((uchar
				*)out_buffer_rgb, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
			QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
			is->m_player->SendGetOneImage(image); //调用激发信号的函数
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
	Q_EMIT SIG_getOneImage(img); //发送信号
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