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

//���ฺ��ѭ������������һ��poller(��demo��ΪEpollPoller)�����𲻶�ѭ����ѯpoller���������õ��¼�
class EventLoop : noncopyable{
public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();  

    //����ѭ��
    void loop();

    //�ر�ѭ��
    //��˶ע��
    // This is not 100% thread safe, if you call through a raw pointer,
    // better to call through shared_ptr<EventLoop> for 100% safety.
    void quit();
   
    TimeStamp pollReturnTime() const { return pollReturnTime_; } //poller����ʱ��ʱ�䣬һ��ָ�¼�����ʱ��ʱ��
    int64_t iteration() const { return iteration_; }


    // �ڵ�ǰ�߳���ֱ��ִ�лص�����
    // �䲽��Ϊ1.���ѵ�ǰ�߳� 2.ִ�лص�����cb
    // ��Ϊ��Ϊpublic�����������߳�Ҳ�ܵ��ã������Ҫ�жϵ������ǲ�����������ͬһ���߳�
    // ���Ϊͬһ���߳�ֱ��ִ�У��������
    void runInLoop(Functor cb);

    // ��ǰ�̵߳Ļص��������У���cb��ӣ���pooling���غ�ִ�еĻص�����
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


    // ��ԾChannel:�����¼�������channel������Щchannels��һ��vector���д洢
    ChannelList activeChannels_;


    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_ ;
};