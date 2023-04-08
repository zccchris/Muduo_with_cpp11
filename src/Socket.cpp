#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
using namespace std;

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in))){
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
}

void Socket::listen(){
    //�������Ӷ��е���󳤶ȣ�linux��Ϊ1024��
    if (0 != ::listen(sockfd_, 1024)){
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}

//��װaccept���������ͻ��˵ĵ�ַ����peeraddr�������ذ󶨺ú���ļ������������ļ���������ʾһ�����ӡ�
int Socket::accept(InetAddress *peeraddr){ 

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof(addr));
    //connfdû�����÷�����
    // int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if (connfd >= 0){
        peeraddr->setSockAddr(addr); //��InetAddress��������sockaddr_in
    }
    return connfd;
}

void Socket::shutdownWrite(){
    if (shutdown(sockfd_, SHUT_WR) < 0){
        LOG_ERROR("shutdownWrite error");
    }
}
void Socket::setTcpNoDelay(bool on){ //������naggle�㷨�������С���ݰ���֧��
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    //SO_REUSEADDR������ͬһ�˿�������ͬһ�������Ķ��ʵ����ֻҪÿ��ʵ������һ����ͬ�ı���IP��ַ���ɡ�
}
void Socket::setReusePort(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    //
}
void Socket::setKeepAlive(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    //TCP�������
}