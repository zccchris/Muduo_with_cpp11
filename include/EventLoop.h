#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

#include "Channel.h"
#include "noncopyable.h"
#include "TimeStamp.h"
#include "Poller.h"

//该类负责循环工作，管理一个poller(该demo里为EpollPoller)，负责不断循环查询poller，并处理获得的事件
class EventLoop : noncopyable{
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


    // 在当前线程中直接执行回调函数
    // 其步骤为1.唤醒当前线程 2.执行回调函数cb
    // 因为其为public函数，其他线程也能调用，因此需要判断调用者是不是与其所属同一个线程
    // 如果为同一个线程直接执行，否则入队
    void runInLoop(Functor cb);

    // 当前线程的回调函数队列，将cb入队，在pooling返回后执行的回调函数
    void queueInLoop(Functor cb);

    size_t queueSize() const;


    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);


    bool isInLoopThread() const { return threadId_ == std::this_thread::get_id();}
    // bool callingPendingFunctors() const { return callingPendingFunctors_; }
    bool eventHandling() const { return eventHandling_; }

 

    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead();  // waked up
    void doPendingFunctors();

    void printActiveChannels() const; // DEBUG

    typedef std::vector<Channel*> ChannelList;

    std::atomic<bool> looping_; /* atomic */
    std::atomic<bool> quit_;
    std::atomic<bool> eventHandling_; /* atomic */
    std::atomic<bool> callingPendingFunctors_; /* atomic */
    int64_t iteration_;
    std::thread::id threadId_;
    TimeStamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;
    // unlike in TimerQueue, which is an internal class,
    // we don't expose Channel to client.
    std::unique_ptr<Channel> wakeupChannel_;


    // 活跃Channel:即有事件发生的channel，将这些channels用一个vector进行存储
    ChannelList activeChannels_;


    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_ ;
};