#pragma once
/***
*	该类的作用，做一个连接，将一个Thread与一个EventLoop进行绑定
***/

#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>


class EventLoop;

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