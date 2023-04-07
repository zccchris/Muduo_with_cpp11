#pragma once

#include "noncopyable.h"

#include "EventLoop.h"
#include "EventLoopThread.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>

/***
*	����Ϊ�̳߳أ�����EventLoopThread
*	����֮����һ��baseLoop�������߳���ѭ��������������̳߳صĴ�С����ֻ��һ���̡߳�
***/
class EventLoopThreadPool : noncopyable{
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
	~EventLoopThreadPool();
	void setThreadNum(int numThreads) { numThreads_ = numThreads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	//baseLoopĬ������ѯ�ķ�ʽ����channel��subloop
	EventLoop* getNextLoop();

	/// ��ϣ�Ķ��Ʒ�ʽ����channel����������
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