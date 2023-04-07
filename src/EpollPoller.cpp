#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"


#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>




    const int kNew = -1;
    const int kAdded = 1;
    const int kDeleted = 2;

//创建epoll描述符
EpollPoller::EpollPoller(EventLoop* loop)
    :Poller(loop)
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    ,events_(kInitEventListSize){

    //如果epoll_create1创建失败会返回-1
    //此时抛出异常
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
    std::move(kNew);
}

EpollPoller::~EpollPoller(){
    //关闭epoll描述符
    ::close(epollfd_);
}


//传入一个channel对象，将这个channel对象
void EpollPoller::updateChannel(Channel* channel){

    //获取channel状态，判断其是否在poller中
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;
    if (index == kNew || index == kDeleted){
        //如果这个channel还没有注册到poller上，需要将其注册到poller上
        int fd = channel->fd();
        if (index == kNew){
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        //如果是已经被删除状态
        else{ 
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else{
        //已经注册到poller上了，更改其状态
        int fd = channel->fd();

        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);

        //根据channel的事件状态，更新poller中感兴趣事件
        if (channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//从Poller中删除channel，并将这个channel的状态
void EpollPoller::removeChannel(Channel* channel){

    //把他从poller中的channellist中删掉
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);

    //把他从当前poller管理的epoll描述符上删掉
    if (index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}



//根据相关operation，更新epoll描述符中关心的事件
void EpollPoller::update(int operation, Channel* channel){
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = channel->events();   //返回的是channel管理的fd中感兴趣的事件        
    int fd = channel->fd();             //返回的是channel管理的fd

    event.data.ptr = channel;

    LOG_TRACE << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    
    //调用epoll_ctl进行更新，如果更新失败，抛出异常
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if (operation == EPOLL_CTL_DEL){
            LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
        else{
            LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
    }
}


//调用一次epoll_wait，在得到返回结果后再调用activeChannels，将发生的事件写入channel中的revent
//同时将有事发生的channel统一记录再activechannels中，方便遍历，统一处理。
//这个activeChannels是属于EventLoop对象的，EventLoop对象负责反复调用poll，进行循环
Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels){
    LOG_TRACE << "fd total count " << channels_.size();

    //events_是一个vector，而这里最终目的是要传入数组首地址
    //events_.begin()是迭代器，*events_.begin()代表第一个元素（对象）&*events.begin()就代表第一个元素的地址。

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    //有事发生
    if (numEvents > 0){
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if (implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    //无事发生
    else if (numEvents == 0){
        LOG_TRACE << "nothing happened";
    }

    //坏了
    else{
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    return now;
}


void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const{

    for (int i = 0; i < numEvents; ++i){
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

        int fd = channel->fd();
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);

        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}
