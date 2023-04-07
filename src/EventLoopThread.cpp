#include "EventLoopThread.h"
#include "EventLoop.h"



EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : loop_(NULL)
    , exiting_(false)
    , thread_()
    , mutex_()
    , cond_()
    , callback_(cb)
    , tid(){
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL){
        loop_->quit();
        thread_->join();
    }
}

/***
*   startLoop����һ�����̣߳����߳�����threadFunc�������ú����Ǵ���һ��EventLoop��
*   ��һ��ָ��ָ����������EventLoop�������ظ�ָ��
*   
***/
EventLoop* EventLoopThread::startLoop(){

    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid = std::this_thread::get_id();
        threadFunc();
    }));

    EventLoop* loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, loop_);
        loop = loop_;
    }

    
    return loop;
}

/***
*   startLoop����һ�����̣߳����߳�����threadFunc�������ú����Ǵ���һ��EventLoop��
*   �����洴�����߳�һһ��Ӧ��
***/
void EventLoopThread::threadFunc(){
    EventLoop loop;


    //���������ʼ�ص�������ɶ��
    if (callback_){
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();

    //EventLoop�˳���~
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = NULL;
}