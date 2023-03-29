#include "Channel.h"
#include <sys/epoll.h>



const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
	:loop_(loop)
	,fd_(fd)
	,events_(0)
	,revents_(0)
	,index_(-1)
	,tied_(false){
}


Channel::~Channel() {}



//暂时不太理解tie有什么用
/**
 * @brief channel的tie方法什么时候调用？一个TcpConnection新连接创建的时候，
 * TcpConnection => Channel;
 */
void Channel::tie(const std::shared_ptr<void>& obj){
	tie_ = obj;
	tied_ = true;
}



/**
 * @brief 当改变channel所对应的fd的events事件后，update负责在poller里面更改
 * fd相应的事件epoll_ctl
 * 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
 * channel和poller通过EventLoop进行连接。
 */

void Channel::update(){	
	loop_->updateChannel(this);

}


/**
 * @brief 在channel所属的EventLoop中，把这个channel给删掉
 */
void Channel::remove(){
	loop_->removeChannel(this);
}



void Channel::handleEvent(TimeStamp receiveTime){
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
            HandleEventWithGuard(receiveTime);
    }
    else
        HandleEventWithGuard(receiveTime);
}


/**
 * @brief 根据poller通知的channel发生的具体事件类型，由channel负责调用具体的回调操作。
 */
void Channel::HandleEventWithGuard(TimeStamp receiveTime){
   
    LOG_INFO("channel HandleEvent revents:%d", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))//EPOLLHUP表示这个设备已经断开连接
    {
        if (closeCallback_)
            closeCallback_();
    }
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
            errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
}
