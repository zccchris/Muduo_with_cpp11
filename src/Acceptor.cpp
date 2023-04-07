#include "Acceptor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include "Logger.h"
#include <unistd.h>
#include "InetAddress.h"
#include <cassert>


//创建一个socket，使用AF_INET协议族
static int createNonblockingSocket(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)
    if(sockfd < 0){
        LOG_FATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}

//创建一个acceptor，acceptor会创建一个监听socket
//将该socket与输入的listenAddr绑定, 封装成channel并将其绑定到一个EventLoop中
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblockingSocket())
    , acceptChannel_(loop, acceptSocket_)
    , listening_(false){
    acceptSocket_.setReuseAddr(true); //设置socket选项  
    acceptSocket_.setReusePort(true); //设置socket选项
    acceptSocket_.bindAddress(listenAddr); //bind
    // TcpServer::start()Acceptor.listen 有新用户连接 执行一个回调 connfd => channel => subloop
    // baseLoop_ 监听到Accpetor有监听事件，baseLoop_就会帮我们新客户连接的回调函数
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

//有可读事件发生，即有新用户连接了，此时调用该函数
void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0){
        if (newConnectionCallback_){
            newConnectionCallback_(connfd, peerAddr);
        }
        else{
            sockets::close(connfd);
        }
    }
    else{
        LOG_SYSERR << "in Acceptor::handleRead";
        //fd耗尽了
        if (errno == EMFILE){
            LOG_ERROR("sockfd reached limit\n");
        }
    }
}
