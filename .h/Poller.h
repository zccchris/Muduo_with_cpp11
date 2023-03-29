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

	//ͳһ�ӿڣ���Ҫ��ѭ���е���
	//����poll��pollerȥ��ѯ��Ҫ������¼�����ŵ�activeChannels��
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

	//����poller�еĸ���Ȥ�¼�
	virtual void updateChannel(Channel* channel) = 0;

	//�Ƴ�channel
	virtual void removeChannel(Channel* channel) = 0;

	//�ж�poller���Ƿ���ĳchannel
	virtual bool hasChannel(Channel* channel) const;

	//����
	static Poller* newDefaultPoller(EventLoop* loop);

protected:
	typedef std::unordered_map<int, Channel*> ChannelMap;
	ChannelMap channels_;

private:
	EventLoop* ownerLoop_;
};