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


//��ʼѭ��
void EventLoop::loop(){

    looping_ = true;
    quit_ = false;

    LOG_TRACE << "EventLoop " << this << " start looping";

    //������߼�Ϊ����һ��Channels�б�activeChannels��պ󴫸�������poller
    //��poller��poll�����н���wait�������¼�����ʱ�����¼�д��activeChannels
    //Ȼ�����activeChannels�е�ÿ��Channel�����������������¼����д���
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
        IO�߳� mainLoop����������û������ӣ�
        mainloopʵ��ע��һ���ص�cb������ص���subloop��ִ�С�
        wakeup subloop��ִ������ķ�����ִ��mainloop��cb������
         */
        doPendingFunctors(); //ִ�е�ǰEventLoop�¼�ѭ����Ҫ����Ļص�������
    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}


/***
*   �˳��¼�ѭ������ΪquitΪһ��public���������Է����������
*   1.�����߳��Լ�������quit����
*       ��ʱ��loop�У���Ϊquit_Ϊtrue������ѭ����
*   2.�ǹ����̵߳�����quit����
*       ���ʱ������߳̿�����Ϊ��loop�е�����poll�����������У���Ҫʹ��wakeup()���л���
*       �൱�ڶ��Ǹ�ѭ������һ��continue
***/
void EventLoop::quit(){
    
    quit_ = true;

    if (!isInLoopThread()){
        wakeup();
    }
}


/***
*   ������ǰEventLoopһ���ص����������䴦���������runInLoop���ǵ�ǰEventLoop�Ĺ����߳�
*   ��˵������ֱ��ִ�иûص�������������õ�ǰEventLoop��runInLoop�Ĳ�����������̣߳����
*   �ûص�����������ŵ����������ȹ������߳������һ��loop�󣬵���doPendingFunctors()ִ��
*   ��������Ҫ����Ļص�������
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

    //��Ϊ����Ҫ�ѻص������ŵ�������ʱ���϶��Ƿǹ����߳̽��еĵ��ã����Ϊ���̰߳�ȫ����Ҫ
    //���м�������
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    //������Ӧ�ģ���Ҫִ������ص�������loop�߳�
    // || callingPendingFunctors_����˼�ǣ���ǰloop����ִ�лص�������loop�������µĻص���
    // ���ʱ���Ҫwakeup()loop�����̣߳���������ȥִ�����Ļص���
    
    //�ҵ����
    //������Ҫ����һ�£�wakeup()��������ǿ�л���������poll�е��̡߳���ʲôʱ����Ҫ�����أ�
    //1.���������Ӻ������̲߳��ǹ����̣߳�˵�������̺ܴ߳����æ�������أ��򲻹��������Ƿ����ڱ�������
    //  ������wakeup()ȥ����������ù����߳̽��е�loop�ĺ���ȥִ��doPendingFunctors()
    //2.��callingPendingFunctors_Ϊtrue����˵�������߳�����ִ�лص�����ʱ�������µĻص���������ˣ���ʱ��
    //  ����wakeup()���ܱ�֤�����߳���ִ���굱ǰ�ص�������һ��ѭ���У�Ҳ����poll������������ִ���µĻص���
    //��ʲôʱ����ǿ�л����أ�
    //�������̵߳���queueInLoop�������䲻��ִ��doPendingFunctors()����ʱ��ʹ����Ҫwakeup��Ҳ��ܿ�ִ�е�doPendingFunctors()
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
*   ��һ��loop�У���Ϊ������poll��������EventLoop�Ĺ����߳̽��ᱻһֱ������û���¼������㲻�᷵��
*   wakeupFd_����һ�����������ֶ��������¼���ͨ��wakeup������wakeupFd_д��8�ֽ�ʹ��д���¼�����
*   �Ӷ�ǿ�л��������е�EventLoop
*   �������ڣ�wakeupchannelʲôʱ��ע�ᵽPoller���أ�
*   
*   ΪʲôҪ���� EventLoop��
*   ���ȣ����������ִ�лص���������ͨ��pendingFunctors_.push_back(cb)
*   ���ú������� pendingFunctors_�С�EventLoop ��ÿһ��ѭ�����������
*   doPendingFunctors ����ִ����Щ�������� EventLoop �Ļ�����ͨ�� epoll_wait ʵ�ֵģ�
*   �����ʱ�� EventLoop �гٳ�û���¼���������ô epoll_wait һֱ�ͻ�������
*   �����ᵼ�£�pendingFunctors_�е�����ٳٲ��ܱ�ִ���ˡ�
*   ���Ա���Ҫ���� EventLoop ���Ӷ���pendingFunctors_�е����񾡿챻ִ�С�
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
*   ��queueInLoop�����У�������pendingFunctors_������������»ص������̴߳���
*   ���������߳������һ��loop֮�󣬻����doPendingFunctors���д���
*   ����������У�������һ���ֲ���functors���飬��������ǿյģ�ֻ��Ҫһ����������
*   ͨ��swap���������߽��н�������pendingFunctors_�����д�����Ļص�����ȫ������functors������
*   ͬʱ��pendingFunctors_�����ÿա�
*   �������ĺô���ʲô��
*   ��ΪqueueInLoop��doPendingFunctors�൱��һ�������ߺ������ߣ�����һ���ԵĽ��������Է�ֹƵ������������
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

