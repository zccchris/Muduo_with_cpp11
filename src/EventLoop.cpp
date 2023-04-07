#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"
#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <memory>
#include <thread>
#include <cassert>


const int kPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0){
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,eventHandling_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(std::this_thread::get_id())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this, wakeupFd_)){
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread){
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
        << " destructs in thread " << threadId_;
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}


//开始循环
void EventLoop::loop(){

    looping_ = true;
    quit_ = false;

    LOG_TRACE << "EventLoop " << this << " start looping";

    //整体的逻辑为，将一个Channels列表activeChannels清空后传给其管理的poller
    //在poller的poll函数中进行wait，当有事件发生时，将事件写入activeChannels
    //然后遍历activeChannels中的每个Channel，对他们所发生的事件进行处理
    while (!quit_){
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        // TODO sort channel by priority
        eventHandling_ = true;
        for (auto channel : activeChannels_){
             channel->handleEvent(pollReturnTime_);
        }
        eventHandling_ = false;
        /**
        IO线程 mainLoop负责接收新用户的连接，
        mainloop实现注册一个回调cb，这个回调由subloop来执行。
        wakeup subloop后执行下面的方法，执行mainloop的cb操作。
         */
        doPendingFunctors(); //执行当前EventLoop事件循环需要处理的回调操作。
    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}


/***
*   退出事件循环，因为quit为一个public函数，所以分两种情况：
*   1.管理线程自己调用了quit函数
*       此时在loop中，因为quit_为true，结束循环。
*   2.非管理线程调用了quit函数
*       这个时候管理线程可能因为在loop中调用了poll而处于阻塞中，需要使用wakeup()进行唤醒
*       相当于对那个循环做了一次continue
***/
void EventLoop::quit(){
    
    quit_ = true;

    if (!isInLoopThread()){
        wakeup();
    }
}


/***
*   传给当前EventLoop一个回调函数，让其处理，如果调用runInLoop的是当前EventLoop的管理线程
*   则说明可以直接执行该回调函数，如果调用当前EventLoop的runInLoop的不是其管理者线程，则把
*   该回调函数的任务放到队列哩，等管理者线程完成了一次loop后，调用doPendingFunctors()执行
*   队列中需要处理的回调操作。
***/
void EventLoop::runInLoop(Functor cb){
    if (isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb){

    //因为当需要把回调函数放到队列中时，肯定是非管理线程进行的调用，因此为了线程安全，需要
    //进行加锁处理。
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    //唤醒相应的，需要执行上面回调操作的loop线程
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调，
    // 这个时候就要wakeup()loop所在线程，让它继续去执行它的回调。
    
    //我的理解
    //这里需要解释一下，wakeup()的作用是强行唤醒阻塞在poll中的线程。而什么时候需要唤醒呢？
    //1.调用这个入队函数的线程不是管理线程，说明管理线程很大可能忙着阻塞呢，则不管其真正是否正在被阻塞，
    //  都调用wakeup()去解除阻塞，让管理线程进行到loop的后面去执行doPendingFunctors()
    //2.当callingPendingFunctors_为true，这说明管理线程正在执行回调函数时，又有新的回调函数入队了，这时候
    //  进行wakeup()，能保证管理线程在执行完当前回调后，在下一次循环中，也不被poll阻塞，而继续执行新的回调。
    //那什么时候不用强行唤醒呢？
    //当管理线程调用queueInLoop，并且其不在执行doPendingFunctors()，此时即使不需要wakeup，也会很快执行到doPendingFunctors()
    if (!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}


void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    if (eventHandling_){
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

/***
*   在一次loop中，因为调用了poll函数，该EventLoop的管理线程将会被一直阻塞，没有事件发生便不会返回
*   wakeupFd_则是一个可以我们手动触发的事件，通过wakeup函数向wakeupFd_写入8字节使得写入事件发生
*   从而强行唤醒阻塞中的EventLoop
*   问题在于，wakeupchannel什么时候注册到Poller上呢？
*   
*   为什么要唤醒 EventLoop？
*   首先，如果来不及执行回调函数，会通过pendingFunctors_.push_back(cb)
*   将该函数放在 pendingFunctors_中。EventLoop 的每一轮循环在最后会调用
*   doPendingFunctors 依次执行这些函数。而 EventLoop 的唤醒是通过 epoll_wait 实现的，
*   如果此时该 EventLoop 中迟迟没有事件触发，那么 epoll_wait 一直就会阻塞。
*   这样会导致，pendingFunctors_中的任务迟迟不能被执行了。
*   所以必须要唤醒 EventLoop ，从而让pendingFunctors_中的任务尽快被执行。
***/

void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = socket::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    size_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}


/***
*   在queueInLoop函数中，我们往pendingFunctors_数组里面插入新回调，待线程处理
*   而当管理线程完成了一次loop之后，会调用doPendingFunctors进行处理
*   在这个函数中，定义了一个局部的functors数组，这个数组是空的，只需要一次锁，便能
*   通过swap函数将两者进行交换，将pendingFunctors_数组中待处理的回调函数全部导入functors待处理
*   同时把pendingFunctors_数组置空。
*   这样做的好处是什么？
*   因为queueInLoop和doPendingFunctors相当于一个生产者和消费者，这样一次性的交换，可以防止频繁的上锁解锁
***/
void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const Functor& functor : functors){
        functor();
    }
    callingPendingFunctors_ = false;
}

