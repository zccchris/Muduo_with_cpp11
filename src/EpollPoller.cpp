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

//����epoll������
EpollPoller::EpollPoller(EventLoop* loop)
    :Poller(loop)
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    ,events_(kInitEventListSize){

    //���epoll_create1����ʧ�ܻ᷵��-1
    //��ʱ�׳��쳣
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
    std::move(kNew);
}

EpollPoller::~EpollPoller(){
    //�ر�epoll������
    ::close(epollfd_);
}


//����һ��channel���󣬽����channel����
void EpollPoller::updateChannel(Channel* channel){

    //��ȡchannel״̬���ж����Ƿ���poller��
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;
    if (index == kNew || index == kDeleted){
        //������channel��û��ע�ᵽpoller�ϣ���Ҫ����ע�ᵽpoller��
        int fd = channel->fd();
        if (index == kNew){
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        //������Ѿ���ɾ��״̬
        else{ 
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else{
        //�Ѿ�ע�ᵽpoller���ˣ�������״̬
        int fd = channel->fd();

        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);

        //����channel���¼�״̬������poller�и���Ȥ�¼�
        if (channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//��Poller��ɾ��channel���������channel��״̬
void EpollPoller::removeChannel(Channel* channel){

    //������poller�е�channellist��ɾ��
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);

    //�����ӵ�ǰpoller�����epoll��������ɾ��
    if (index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}



//�������operation������epoll�������й��ĵ��¼�
void EpollPoller::update(int operation, Channel* channel){
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = channel->events();   //���ص���channel�����fd�и���Ȥ���¼�        
    int fd = channel->fd();             //���ص���channel�����fd

    event.data.ptr = channel;

    LOG_TRACE << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    
    //����epoll_ctl���и��£��������ʧ�ܣ��׳��쳣
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if (operation == EPOLL_CTL_DEL){
            LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
        else{
            LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
    }
}


//����һ��epoll_wait���ڵõ����ؽ�����ٵ���activeChannels�����������¼�д��channel�е�revent
//ͬʱ�����·�����channelͳһ��¼��activechannels�У����������ͳһ����
//���activeChannels������EventLoop����ģ�EventLoop�����𷴸�����poll������ѭ��
Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels){
    LOG_TRACE << "fd total count " << channels_.size();

    //events_��һ��vector������������Ŀ����Ҫ���������׵�ַ
    //events_.begin()�ǵ�������*events_.begin()�����һ��Ԫ�أ�����&*events.begin()�ʹ����һ��Ԫ�صĵ�ַ��

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    //���·���
    if (numEvents > 0){
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if (implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    //���·���
    else if (numEvents == 0){
        LOG_TRACE << "nothing happened";
    }

    //����
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
