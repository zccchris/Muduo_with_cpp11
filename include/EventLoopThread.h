#pragma once
/***
*	该类的作用，做一个连接，将一个Thread与一个EventLoop进行绑定
***/

#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>

#include"EventLoop.h"

/***
 *  EventLoopThread封装一个线程创建的过程
 *  当调用startLoop函数时，会创建一个线程，在线程函数中，创建一个EventLoop，并执行该EventLoop的loop
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