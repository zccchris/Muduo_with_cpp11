#pragma once

#include<functional>
#include<string>
#include<memory>
#include "TimeStamp.h"
#include "EventLoop.h"
#include "Socket.h"

/***
*	Channel�ฺ���װһ��socketfd�Լ��������Ȥ���¼������洢�������������¼�
*	ʵ��Channel����ʱ����EventLoop��socketfd���а�
*	��ʼ���󣬵���setReadCallback��setWriteCallback��setCloseCallback��setErrorCallback�ĸ����������䷢����Ӧ�¼�ʱ�Ļص�����
*	��������ͨ��һЩ�к������ø�fd��Ӧ���¼�״̬��������Ҫ�������fd�Ŀɶ��¼�/��д�¼�������ȡ������������Ȥ�¼��б����revent��
*	������������ã���Ϊ��������poller����EventLoop�����Ի����update()����������״̬������EventLoop�е�poller��
*	�ڼ������¼������󣬻Ὣ�������¼�����revents_��
*	handleEvent������ȥ��ѯrevents_�������ݾ��巢�����¼����������úõ���Ӧ�Ļص�����
*	����Channelʱ�ͽ�Channelע�ᵽһ��EventLoop�ϣ�֮�����enableϵ�к������̽��¼�ע����EventLoop�����epoll_fd��
***/
class Channel :public noncopyable {

public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(TimeStamp)> ReadEventCallback;

	//��װһ������socket��������Ȥ���¼�����������ص������EvenLoop��
	Channel(EventLoop* loop, int fd);
	~Channel();

	//
	void handleEvent(TimeStamp receiveTime);

	//���ô����¼��õĻص�����
	//ʹ��std::move����cb���������Ϊ��ֵ���ٰ󶨸���Ӧ���󣬲���Ҫ�پ���һ�θ��ƣ��ƶ�����
	void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
	void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
	void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
	void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }



	void tie(const std::shared_ptr<void>&);//��ֹ��channel���ֶ�remove����channel������ִ�лص�����
	int fd() const { return fd_; }
	int events() const { return events_; }

	//ͨ��������õ�֪��ǰchannel��fd������ʲô��Ҫ������¼�����
	int set_revents(int revt) { revents_ = revt; }


	//����fd��Ӧ���¼�״̬��������Ҫ�������fd�Ŀɶ��¼�/��д�¼�������ȡ��������
	void enableReading() { events_ |= kReadEvent; update(); }
	void disableReading() { events_ &= ~kReadEvent; update(); }
	void enableWriting() { events_ |= kWriteEvent; update(); }
	void disableWriting() { events_ &= ~kWriteEvent; update(); }
	void disableAll() { events_ = kNoneEvent; update(); }

	bool isWriting() const { return events_ & kWriteEvent; } //��ǰ����Ȥ���¼��Ƿ������д�¼�
	bool isReading() const { return events_ & kReadEvent; } //��ǰ����Ȥ���¼��Ƿ�����ɶ��¼�
	bool isNoneEvent() const { return events_ == kNoneEvent; }


	int index() { return index_; }
	void set_index(int idx) { index_ = idx; }

	//one loop per thread 
	EventLoop* ownerLoop() { return loop_; } //��ǰ���channel������һ��event loop
	void remove();




private:

	void update();


	static std::string eventsToString(int fd, int ev);

	void update();
	void handleEventWithGuard(TimeStamp receiveTime); //�����ܱ������¼�

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop* loop_;//��channel���ĸ�EventLoop������
	const int fd_;	//�����Ķ���
	int events_;	//�������¼�
	int revents_;	//����ʱ���������¼��б�

	int index_; //���index_��ʵ��ʾ�������channel��״̬����kNew����kAdded����kDeleted
				//kNew�������channelδ��ӵ�poller�У�kAdded��ʾ����ӵ�poller�У�kDeleted��ʾ��poller��ɾ����

	std::weak_ptr<void> tie_; //���tie_ ����....
	bool tied_;				//�Ƿ񱻰���

	//channelͨ�������ܹ���֪fd���շ����ľ����¼�revents��������������þ����¼��Ļص�������
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};