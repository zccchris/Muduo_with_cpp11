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
*   startLoop创建一个新线程，新线程运行threadFunc函数，该函数是创建一个EventLoop。
*   用一个指针指向函数创建的EventLoop，并返回该指针
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
*   startLoop创建一个新线程，新线程运行threadFunc函数，该函数是创建一个EventLoop，
*   与上面创建的线程一一对应。
***/
void EventLoopThread::threadFunc(){
    EventLoop loop;


    //设置这个初始回调函数干啥？
    if (callback_){
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();

    //EventLoop退出咯~
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = NULL;
}