#pragma once
/*
	��������˶�Epoll�ķ�װ
*/
#include <vector>
#include "Poller.h"
#include "TimeStamp.h"


class EpollPoller : public Poller {
	EpollPoller(EventLoop* loop);
	~EpollPoller() override;


	//��д���෽��
	//����poll����epoll�м�����epoll_wait
	TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;
	//����poller�еĸ���Ȥ�¼�
	void updateChannel(Channel* channel) override;
	//�Ƴ�channel
	void removeChannel(Channel* channel) override;

private:
	//�¼��б��ȣ���ʼΪ16
	static const int kInitEventListSize = 16;
	
	//����Ծ���¼���д��activeChannels��
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	
	//����poller�е�channel
	void update(int operation, Channel* channel);

	typedef std::vector<struct epoll_event> EventList;
	int epollfd_;
	EventList events_;
};