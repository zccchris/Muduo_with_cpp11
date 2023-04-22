#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <string>

static EventLoop* CheckLoopNotNull(EventLoop* loop) //���ﶨ��Ϊstatic��TcpConnection��TcpServer���������������ͻ
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null \b", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}


TcpConnection::TcpConnection(EventLoop* loop, const std::string &nameArg, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnected)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024){
        
    //��channel������Ӧ�Ļص�������poller��channel֪ͨ����Ȥ���¼������ˣ�channel��ص���Ӧ�Ĳ���
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    // LOG_INFO("TcpConnection::creator[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::deletor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)//�������� ע������һ�����ǿ��ת��
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        { //��ʵ���������ͨ�ſ�����棬��ΪTcpConnection��Channel�϶���ע�ᵽ
          //һ��EventLoop���棬���send�϶����ڶ�Ӧ��EventLoop�߳�����ִ�е�
          //������һЩӦ�ó������ܻ��connection��¼�������������̵߳���connection��send��Ҳ���п��ܵ�
          sendInLoop(buf.c_str(), buf.size());
        }
        else{
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,
                                        this,
                                        buf.c_str(),
                                        buf.size()));
        }
    }
}
void TcpConnection::sendInLoop(const void* data, size_t len)
{   //��Buffer������
    //�������� Ӧ��д�Ŀ죬���ں˷�������������Ҫ�Ѵ���������д�뻺����������������ˮλ�ص�
    ssize_t nwrote = 0;
    size_t remaining = len; //��û���͵����ݵĳ���
    bool faultError = false; //��¼�Ƿ��������
    if(state_ == kDisconnected)
    {
        //֮ǰ���ù���connection��shutdown�������ٽ��з�����
        LOG_ERROR("disconnected, give up writing");
        return ;
    }
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        //channel��һ�ο�ʼд���ݣ������û��ռ�ķ��ͻ������л�û�д���������
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0)
        {   
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_)
            {//��Ȼһ���Է������˾Ͳ����ٸ�channel����epollout�¼��ˣ�����������enableWriting
            //����epoll_wait�Ͳ��ü�����д�¼�����ִ��handleWrite�ˡ����������Ч�ʰ�
                loop_->queueInLoop(bind(writeCompleteCallback_, shared_from_this()));
                //�����������˾͵��÷������Ĵ�������
            }
        }
        else
        { //һ����û����dataȫ��������socket���ͻ�������
            nwrote = 0; //
            if(errno != EWOULDBLOCK) 
            {//����Ƿ�����ģʽ��socket���ͻ��������˾ͻ��������ز�������EWOULDBLOCK
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET) //���յ��Զ˵�socket����
                    faultError = true;
            }
        }
    }
    if(!faultError && remaining > 0)
    {
        //˵���ղŵ�writeû�а�����ȫ��������socket���ͻ������У�ʣ���������Ҫ���浽�û���outputBuffer���������С�
        //Ȼ���channelע��epollout�¼���poller����tcp�ķ��ͻ������пռ䣬��֪ͨ��Ӧ��sock channel
        //����handleWrite�ص��������ѷ��ͻ������е�����ȫ��������ɡ�
        size_t oldLen = outputBuffer_.readableBytes();//Ŀǰ���ͻ�����Bufferʣ��Ĵ���������(��������socket������)�ĳ���
        if(oldLen +remaining >= highWaterMark_
            && oldLen < highWaterMark_ //???????????
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if(!channel_->isWriting())
            channel_->enableWriting();//����һ��Ҫע��channel��д�¼�������poller�����channel֪ͨpollout
    }
}
void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting); //�������ó�kDisconnecting�����㻺������������û����
        //��handleWrite�����У�������������ݻ���state_�ǲ���KDisconnecting������Ǿ���ΪKDisconnected
        //
        loop_->runInLoop(bind(&TcpConnection::shutdownInLoop, this));
    }
    
    
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())
    {
        //˵����ǰoutputBuffer�е�����ȫ���������
        socket_->shutdownWrite(); //�ر�д�� ����Channel��EPOLLHUP
        
    }
}


// ���ӽ���
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    //tie(const std::shared_ptr<void> &obj);
    //shared_from_this()���ص���shared_ptr<TcpConnection>�����ܱ�tie�������ܣ�
    //���shared_ptr<void>ʵ���ϵײ����һ��void*ָ�룬��shared_ptr<TcpConnection>
    //ʵ������һ��TcpConnection*ָ��
    //Channel��������һ��weak_ptr��ָ�������������shared_ptr<TcpConnection>
    //������TcpConnection�Ѿ����ͷ��ˣ���ôChannel���е�weak_ptr��û�취��
    //Channel::handleEvent ��û������weak_ptr��
    /**
     * @brief �ҵ��Ǿ������˼��ܲ��������TcpConnection������shared_ptr����ʽ
     * ���뵽Channel�б��󶨣�Ȼ��Channel��ͨ������handleEvent��Channel���е�
     * weak_ptr����Ϊshared_ptr��ͬ�������TcpConnection���������Ļ�����ʹ
     * ��������TcpConnection����ָ�뱻�ͷ�����ɾ����Channel�����滹��һ������ָ��
     * ָ������������TcpConnection����Ҳ���ᱻ�ͷš���Ϊ���ü���û�б�Ϊ0.
     * ���˼�볬���ã���ֹ������ɵúúõģ����ȴͻȻ���㸪�׳�н
     * 
     */
    channel_->enableReading(); //��pollerע��channel��epollin�¼�
    //�����ӽ�����ִ�лص�
    connectionCallback_(shared_from_this());

}



void TcpConnection::handleRead(TimeStamp receiveTime){
    int savedErrno = 0; 
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);//�����channel��fdҲһ������socket fd
    if(n > 0) //��fd���������ݣ����ҷ�����inputBuffer_��
    {
        //�ѽ������ӵ��û����пɶ��¼������ˣ������û�����Ļص�����onMessage
        //���shared_from_this()����TcpConnection���������ָ��
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
        handleClose();
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting()) //��ǰ����Ȥ���¼��Ƿ������д�¼�������
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);//ͨ��fd��������
        if(n > 0) //n > 0˵����Bufferд��ɹ���Buffer��Ҫ����ȥ��socket������
        {
            outputBuffer_.retrieve(n);//����ɹ�д���ˣ���Ҫһ��һ��readerIndex;�Ҿ��ð���仰��װ��writeFd���ð�
            if(outputBuffer_.readableBytes() == 0)
            { //���˵Buffer�����Ѿ�û�������ˣ�ȫ�����������ͻ�������
                channel_->disableWriting(); //��ô�͹ر����channel�Ŀ�д�¼���
                //���Ʋ⣬��Buffer���������ˣ���������д�¼���������������һ���ô�������
                //�����ܵĽ�epoll_wait�����ɶ��رյ��¼��ļ���������Ƶ��������д�¼������Ч�ʣ�
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );//queueInLoop���������loop��Ӧ��thread�߳���ִ�лص�����ʵ�Ҿ�������Ҳ������runInLoop����ȷҲ������
                }
                if(state_ == kDisconnecting) //���ڹر���
                {
                    shutdownInLoop();
                }
            }
        }
        else{
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else{
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n",channel_->fd(), static_cast<int>(state_));
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());

    //��ʵ�Ҿ�������Ӧ�ü����ж���������һû����ִ�йر����ӵĻص�������ô��
    connectionCallback_(connPtr); //ִ�����ӹرյĻص� ��������������������������������������������
    closeCallback_(connPtr); //�ر�����
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;

    //��Linux�����ܷ�������̡�page88����ȡ�����socket����״̬,getsockopt�ɹ��򷵻�0,ʧ���򷵻�-1
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen)<0)
        err = errno;
    else
        err = optval;
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);

}

// ��������
void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();//��channel��poller��ɾ������
}
