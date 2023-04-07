#pragma once
/*
	该类完成了对Epoll的封装
*/
#include <vector>
#include "Poller.h"
#include "TimeStamp.h"


class EpollPoller : public Poller {
	EpollPoller(EventLoop* loop);
	~EpollPoller() override;


	//重写基类方法
	//运行poll，在epoll中即进行epoll_wait
	TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;
	//更新poller中的感兴趣事件
	void updateChannel(Channel* channel) override;
	//移除channel
	void removeChannel(Channel* channel) override;

private:
	//事件列表长度，初始为16
	static const int kInitEventListSize = 16;
	
	//将活跃的事件填写到activeChannels中
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	
	//更新poller中的channel
	void update(int operation, Channel* channel);

	typedef std::vector<struct epoll_event> EventList;
	int epollfd_;
	EventList events_;
};