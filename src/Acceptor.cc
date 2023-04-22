#include "Acceptor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include "Logger.h"
#include <unistd.h>
#include "InetAddress.h"
#include <cassert>


//����һ��socket��ʹ��AF_INETЭ����
static int createNonblockingSocket(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_FATAL("sockets::createNonblockingOrDie");
    }
    return sockfd;
}


Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblockingSocket())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listening_(false){
    acceptSocket_.setReuseAddr(true); //����socketѡ��  
    acceptSocket_.setReusePort(true); //����socketѡ��
    acceptSocket_.bindAddress(listenAddr); //bind
    // TcpServer::start()Acceptor.listen �����û����� ִ��һ���ص� connfd => channel => subloop
    // baseLoop_ ������Accpetor�м����¼���baseLoop_�ͻ�������¿ͻ����ӵĻص�����
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0){
        if (newConnectionCallback_){
            newConnectionCallback_(connfd, peerAddr);
        }
        else{
            ::close(connfd);
        }
    }
    else{
        //LOG_SYSERR << "in Acceptor::handleRead";
        //fd�ľ���
        if (errno == EMFILE){
            LOG_ERROR("sockfd reached limit\n");
        }
    }
}