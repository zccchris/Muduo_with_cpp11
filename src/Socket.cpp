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
    //请求连接队列的最大长度，linux中为1024。
    if (0 != ::listen(sockfd_, 1024)){
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}

//封装accept函数，将客户端的地址存入peeraddr，并返回绑定好后的文件描述符，该文件描述符表示一个连接。
int Socket::accept(InetAddress *peeraddr){ 

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof(addr));
    //connfd没有设置非阻塞
    // int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if (connfd >= 0){
        peeraddr->setSockAddr(addr); //给InetAddress对象设置sockaddr_in
    }
    return connfd;
}

void Socket::shutdownWrite(){
    if (shutdown(sockfd_, SHUT_WR) < 0){
        LOG_ERROR("shutdownWrite error");
    }
}
void Socket::setTcpNoDelay(bool on){ //不启用naggle算法，增大对小数据包的支持
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    //SO_REUSEADDR允许在同一端口上启动同一服务器的多个实例，只要每个实例捆绑一个不同的本地IP地址即可。
}
void Socket::setReusePort(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    //
}
void Socket::setKeepAlive(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    //TCP保活机制
}