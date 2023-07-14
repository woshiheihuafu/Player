#include "packetqueue.h"
//��ʼ������
void packet_queue_init(PacketQueue* queue)
{
	queue->first_pkt = NULL;
	queue->last_pkt = NULL;
	queue->nb_packets = 0;
	queue->size = 0;
	queue->mutex = SDL_CreateMutex();
	queue->cond = SDL_CreateCond();
}
//��� -- β���
int packet_queue_put(PacketQueue* queue, AVPacket* packet)
{
	AVPacketList* pkt_list;
	// ��У��
	if (av_dup_packet(packet) < 0)
	{
		return -1;
	}
	pkt_list = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (pkt_list == NULL)
	{
		return -1;
	}
	pkt_list->pkt = *packet;
	pkt_list->next = NULL;
	//����
	SDL_LockMutex(queue->mutex);
	if (queue->last_pkt == NULL) //�ն�
	{
		queue->first_pkt = pkt_list;
	}
	else
	{
		queue->last_pkt->next = pkt_list;
	}
	queue->last_pkt = pkt_list;
	queue->nb_packets++;
	queue->size += packet->size;
	SDL_CondSignal(queue->cond); //����귢�������������ź�--û���ź�����(������������)
	SDL_UnlockMutex(queue->mutex);
	return 0;
}
/// ����--ͷɾ��
/// queue �������ָ�� pkt ������͵Ĳ���, ���ؽ��.
/// block ��ʾ�Ƿ����� Ϊ 1 ʱ ����Ϊ�������ȴ�
int packet_queue_get(PacketQueue* queue, AVPacket* pkt, int block)
{
	AVPacketList* pkt_list = NULL;
	int ret = 0;
	SDL_LockMutex(queue->mutex);
	while (1)
	{
		pkt_list = queue->first_pkt;
		if (pkt_list != NULL) //�Ӳ��գ���������
		{
			queue->first_pkt = queue->first_pkt->next; //pkt_list->next
			if (queue->first_pkt == NULL)
			{
				queue->last_pkt = NULL;
			}
			queue->nb_packets--;
			queue->size -= pkt_list->pkt.size;
			*pkt = pkt_list->pkt; // ���Ƹ� packet��
			av_free(pkt_list);
			ret = 1;
			break;
		}
		else if (block == 0)
		{
			ret = 0;
			break;
		}
		else
		{
			SDL_CondWait(queue->cond, queue->mutex);
		}
	}
	SDL_UnlockMutex(queue->mutex);
	return ret;
}