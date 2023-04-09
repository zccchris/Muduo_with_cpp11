#pragma once

#include<functional>
#include<string>
#include<memory>
#include "TimeStamp.h"
#include "EventLoop.h"
#include "Socket.h"

/***
*	Channel类负则封装一个socketfd以及对其感兴趣的事件，并存储真正监听到的事件
*	实例Channel对象时传入EventLoop和socketfd进行绑定
*	初始化后，调用setReadCallback，setWriteCallback，setCloseCallback，setErrorCallback四个函数设置其发生相应事件时的回调函数
*	接下来，通过一些列函数设置该fd相应的事件状态，比如需要监听这个fd的可读事件/可写事件，或者取消监听，感兴趣事件列表存在revent中
*	对于上面的设置，因为真正管理poller的是EventLoop，所以会调用update()函数将上述状态更新至EventLoop中的poller中
*	在监听到事件发生后，会将发生的事件存入revents_中
*	handleEvent函数会去查询revents_，并根据具体发生的事件调用已设置好的相应的回调函数
*	创建Channel时就将Channel注册到一个EventLoop上，之后调用enable系列函数即刻将事件注册至EventLoop管理的epoll_fd中
***/
class Channel :public noncopyable {

public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(TimeStamp)> ReadEventCallback;

	//封装一个连接socket与对其感兴趣的事件，并将其挂载到传入的EvenLoop中
	Channel(EventLoop* loop, int fd);
	~Channel();

	//
	void handleEvent(TimeStamp receiveTime);

	//设置处理事件用的回调函数
	//使用std::move，将cb这个函数变为右值后再绑定给相应对象，不需要再经历一次复制，移动语义
	void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
	void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
	void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
	void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }



	void tie(const std::shared_ptr<void>&);//防止当channel被手动remove掉，channel还正在执行回调操作
	int fd() const { return fd_; }
	int events() const { return events_; }

	//通过这个设置得知当前channel的fd发生了什么需要处理的事件类型
	int set_revents(int revt) { revents_ = revt; }


	//设置fd相应的事件状态，比如需要监听这个fd的可读事件/可写事件，或者取消监听。
	void enableReading() { events_ |= kReadEvent; update(); }
	void disableReading() { events_ &= ~kReadEvent; update(); }
	void enableWriting() { events_ |= kWriteEvent; update(); }
	void disableWriting() { events_ &= ~kWriteEvent; update(); }
	void disableAll() { events_ = kNoneEvent; update(); }

	bool isWriting() const { return events_ & kWriteEvent; } //当前感兴趣的事件是否包含可写事件
	bool isReading() const { return events_ & kReadEvent; } //当前感兴趣的事件是否包含可读事件
	bool isNoneEvent() const { return events_ == kNoneEvent; }


	int index() { return index_; }
	void set_index(int idx) { index_ = idx; }

	//one loop per thread 
	EventLoop* ownerLoop() { return loop_; } //当前这个channel属于哪一个event loop
	void remove();




private:

	void update();


	static std::string eventsToString(int fd, int ev);

	void update();
	void handleEventWithGuard(TimeStamp receiveTime); //处理受保护的事件

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop* loop_;//该channel在哪个EventLoop对象中
	const int fd_;	//监听的对象
	int events_;	//监听的事件
	int revents_;	//返回时监听到的事件列表

	int index_; //这个index_其实表示的是这个channel的状态，是kNew还是kAdded还是kDeleted
				//kNew代表这个channel未添加到poller中，kAdded表示已添加到poller中，kDeleted表示从poller中删除了

	std::weak_ptr<void> tie_; //这个tie_ 绑定了....
	bool tied_;				//是否被绑定了

	//channel通道里面能够获知fd最终发生的具体事件revents，所以它负责调用具体事件的回调操作。
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};