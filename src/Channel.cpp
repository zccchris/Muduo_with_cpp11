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



//��ʱ��̫���tie��ʲô��
/**
 * @brief channel��tie����ʲôʱ����ã�һ��TcpConnection�����Ӵ�����ʱ��
 * TcpConnection => Channel;
 */
void Channel::tie(const std::shared_ptr<void>& obj){
	tie_ = obj;
	tied_ = true;
}



/**
 * @brief ���ı�channel����Ӧ��fd��events�¼���update������poller�������
 * fd��Ӧ���¼�epoll_ctl
 * ͨ��channel������EventLoop������poller����Ӧ������ע��fd��events�¼�
 * channel��pollerͨ��EventLoop�������ӡ�
 */

void Channel::update(){	
	loop_->updateChannel(this);

}


/**
 * @brief ��channel������EventLoop�У������channel��ɾ��
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
 * @brief ����poller֪ͨ��channel�����ľ����¼����ͣ���channel������þ���Ļص�������
 */
void Channel::HandleEventWithGuard(TimeStamp receiveTime){
   
    LOG_INFO("channel HandleEvent revents:%d", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))//EPOLLHUP��ʾ����豸�Ѿ��Ͽ�����
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
