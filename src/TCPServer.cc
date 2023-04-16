#include "TCPServer.h"
#include "Logger.h"
#include<cassert>
#include<string>

/***
 *  @brief �ж�loop�Ƿ�Ϊ��
 *  @return ������յĻ�����ԭloop
 ***/
static EventLoop* CheckLoopNotNull(EventLoop* loop) //���ﶨ��Ϊstatic��TcpConnection��TcpServer���������������ͻ
{
    if (loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null \b", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))              //�����µ��̳߳�
    , connectionCallback_(defaultConnectionCallback)                 //�������ӿɶ��¼��Ļص�����
    , messageCallback_(defaultMessageCallback)                       //����Ϣ�¼�����ʱ�Ļص�����
    , nextConnId_(1){
        
        acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer(){
    // LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";
    for (auto& item : connections_){
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start(){
    if (started_++ == 0){
        threadPool_->start(threadInitCallback_);

        assert(!acceptor_->listening());
        loop_->runInLoop(
            std::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}
/*
   һ�������ӵ���ʱӦ����δ���
   ���ݴ����sockfd(��������)��peerAddr(�ͻ��˵�ַ)������һ��TcpConnection����
 */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr){
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    // LOG_INFO << "TcpServer::newConnection [" << name_
    //     << "] - new connection [" << connName
    //     << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
        << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}
