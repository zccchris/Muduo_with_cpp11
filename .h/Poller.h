#pragma once


#include "noncopyable.h"
#include"Channel.h"

#include <unordered_map>
#include <vector>


class Poller : public muduo::noncopyable {
public:
	typedef std::vector<Channel*> ChannelList;

	Poller(EventLoop* loop);
	virtual ~Poller();

	//统一接口，需要在循环中调用
	//启动poll，poller去查询需要处理的事件，存放到activeChannels中
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

	//更新poller中的感兴趣事件
	virtual void updateChannel(Channel* channel) = 0;

	//移除channel
	virtual void removeChannel(Channel* channel) = 0;

	//判断poller中是否有某channel
	virtual bool hasChannel(Channel* channel) const;

	//不懂
	static Poller* newDefaultPoller(EventLoop* loop);

protected:
	typedef std::unordered_map<int, Channel*> ChannelMap;
	ChannelMap channels_;

private:
	EventLoop* ownerLoop_;
};