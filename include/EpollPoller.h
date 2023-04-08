#pragma once
/*
	��������˶�Epoll�ķ�װ
*/
#include <vector>
#include "Poller.h"
#include "TimeStamp.h"


/***
 *	EpollPoller��װ��Epoll������һ��EpollPoller����ʱ����Ҫ��¼������EventLoop
 *	���У�
 *	���캯���з�װ��epoll_create1
 *  update()��װ��epoll_ctl��poll()��װ��epoll_wait
 * 
 * 	ͨ��updateChannel��removeChannel�����ǿ��Խ�channel����������Ȥ���¼�����epoll_set�Ͻ��и��£�ɾ�������
 * 	���������º�Channel��״̬
 * 	
***/

class EpollPoller : public Poller {
	EpollPoller(EventLoop* loop);
	~EpollPoller() override;


	//��д���෽��


	//����һ��epoll_wait���ڵõ����ؽ�����ٵ���activeChannels�����������¼�д��channel�е�revent
	//ͬʱ�����·�����channelͳһ��¼����������EventLoop��activechannels�У����������ͳһ����
	//EventLoop�����𷴸�����poll������ѭ����
	//@prama timeoutMs Ϊepoll_wait�ĵȴ��¼�����msΪ��λ
	//@prama activeChannelsΪ���շ����¼���Channel��Channel����
	TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;


	//����poller�еĸ���Ȥ�¼�
	void updateChannel(Channel* channel) override;
	//�Ƴ�channel
	void removeChannel(Channel* channel) override;

private:
	//�¼��б��ȣ���ʼΪ16
	static const int kInitEventListSize = 16;
	
	//����Ծ���¼���д��activeChannels��
	//@prama numEventsΪepoll_wait���صľ���socket_fd�ĸ���
	//@prama activeChannelsΪEventLoop����Ļ�ԾChannel����
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	
	//����poller�е�channel
	void update(int operation, Channel* channel);

	typedef std::vector<struct epoll_event> EventList;
	int epollfd_;
	EventList events_;
};