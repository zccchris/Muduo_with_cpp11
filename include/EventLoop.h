#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>

#include "Channel.h"
#include "noncopyable.h"
#include "TimeStamp.h"
#include "Poller.h"



//该类负责循环工作，管理一个poller，负责不断循环查询poller，并处理获得的事件
class EventLoop : noncopyable
{
public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();  

    //开启循环
    void loop();

    //关闭循环
    //陈硕注：
    // This is not 100% thread safe, if you call through a raw pointer,
    // better to call through shared_ptr<EventLoop> for 100% safety.
    void quit();
   
    TimeStamp pollReturnTime() const { return pollReturnTime_; } //poller返回时的时间，一般指事件到达时的时间
    int64_t iteration() const { return iteration_; }

    /// Runs callback immediately in the loop thread.
    /// It wakes up the loop, and run the cb.
    /// If in the same loop thread, cb is run within the function.
    /// Safe to call from other threads.
    void runInLoop(Functor cb);
    /// Queues callback in the loop thread.
    /// Runs after finish pooling.
    /// Safe to call from other threads.
    void queueInLoop(Functor cb);

    size_t queueSize() const;

    // internal usage
    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);


    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    // bool callingPendingFunctors() const { return callingPendingFunctors_; }
    bool eventHandling() const { return eventHandling_; }

 

    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead();  // waked up
    void doPendingFunctors();

    void printActiveChannels() const; // DEBUG

    typedef std::vector<Channel*> ChannelList;

    bool looping_; /* atomic */
    std::atomic<bool> quit_;
    bool eventHandling_; /* atomic */
    bool callingPendingFunctors_; /* atomic */
    int64_t iteration_;
    const pid_t threadId_;
    TimeStamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;
    // unlike in TimerQueue, which is an internal class,
    // we don't expose Channel to client.
    std::unique_ptr<Channel> wakeupChannel_;


    // scratch variables
    ChannelList activeChannels_;


    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_ ;
};