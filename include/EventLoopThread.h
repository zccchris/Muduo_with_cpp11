#pragma once
/***
*	��������ã���һ�����ӣ���һ��Thread��һ��EventLoop���а�
***/

#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>

#include"EventLoop.h"

/***
 *  EventLoopThread��װһ���̴߳����Ĺ���
 *  ������startLoop����ʱ���ᴴ��һ���̣߳����̺߳����У�����һ��EventLoop����ִ�и�EventLoop��loop
 * 
 * 
***/
class EventLoopThread : public noncopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = std::string());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    std::shared_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
    std::thread::id tid;
};