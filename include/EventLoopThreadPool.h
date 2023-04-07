#pragma once

#include "noncopyable.h"

#include "EventLoop.h"
#include "EventLoopThread.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>

/***
*	该类为线程池，管理EventLoopThread
*	创建之初有一个baseLoop，在主线程中循环，如果不设置线程池的大小，则只有一个线程。
***/
class EventLoopThreadPool : noncopyable{
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
	~EventLoopThreadPool();
	void setThreadNum(int numThreads) { numThreads_ = numThreads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	//baseLoop默认以轮询的方式分配channel给subloop
	EventLoop* getNextLoop();

	/// 哈希的定制方式分配channel，后续补充
	EventLoop* getLoopForHash(size_t hashCode);

	std::vector<EventLoop*> getAllLoops();

	bool started() const{
		return started_;
	}

	const std::string& name() const{
		return name_;
	}

private:

	EventLoop* baseLoop_;
	std::string name_;
	bool started_;
	int numThreads_;
	int next_;
	std::vector<std::unique_ptr<EventLoopThread>> threads_;
	std::vector<EventLoop*> loops_;
}; 