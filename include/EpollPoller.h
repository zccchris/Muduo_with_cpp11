#pragma once
/*
	该类完成了对Epoll的封装
*/
#include <vector>
#include "Poller.h"
#include "TimeStamp.h"


/***
 *	EpollPoller封装了Epoll，创建一个EpollPoller对象时，需要记录其所属EventLoop
 *	其中：
 *	构造函数中封装了epoll_create1
 *  update()封装了epoll_ctl，poll()封装了epoll_wait
 * 
 * 	通过updateChannel和removeChannel，我们可以将channel及其所感兴趣的事件，在epoll_set上进行更新，删除或插入
 * 	并调整更新后Channel的状态
 * 	
***/

class EpollPoller : public Poller {
	EpollPoller(EventLoop* loop);
	~EpollPoller() override;


	//重写基类方法


	//调用一次epoll_wait，在得到返回结果后再调用activeChannels，将发生的事件写入channel中的revent
	//同时将有事发生的channel统一记录在其所属的EventLoop的activechannels中，方便遍历，统一处理。
	//EventLoop对象负责反复调用poll，进行循环，
	//@prama timeoutMs 为epoll_wait的等待事件，以ms为单位
	//@prama activeChannels为接收发生事件的Channel的Channel链表
	TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;


	//更新poller中的感兴趣事件
	void updateChannel(Channel* channel) override;
	//移除channel
	void removeChannel(Channel* channel) override;

private:
	//事件列表长度，初始为16
	static const int kInitEventListSize = 16;
	
	//将活跃的事件填写到activeChannels中
	//@prama numEvents为epoll_wait返回的就绪socket_fd的个数
	//@prama activeChannels为EventLoop管理的活跃Channel队列
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	
	//更新poller中的channel
	void update(int operation, Channel* channel);

	typedef std::vector<struct epoll_event> EventList;
	int epollfd_;
	EventList events_;
};