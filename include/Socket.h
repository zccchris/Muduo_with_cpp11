#pragma once
#include "noncopyable.h"
#include "InetAddress.h"


/***
 *	@brief 该类封装了一个文件描述符sockfd及应用于该文件描述符的方法
 ***/
class Socket : public noncopyable{
public:
    /***
    *	@brief 构造函数，通过传入sockfd构造一个Socket对象
    *   @param sockfd 用于构造Socket对象的sockfd
    ***/
    explicit Socket(int sockfd) : sockfd_(sockfd){}


    /***
    *	@brief 析构函数
    ***/
    ~Socket();

public:


    /***
    *	@brief 获取该Socket对象封装的sockfd
    *   @return 该Socket对象封装的sockfd
    ***/
    int fd() const {return sockfd_;}


    /***
    *	@brief 封装bind，调用bind将内部sockfd与指定地址进行绑定
    *   @param localaddr 需要绑定到sockfd上的地址
    ***/
    void bindAddress(const InetAddress &localaddr); //调用bind绑定服务器IP端口

    /***
    *	@brief 封装listen，调用listen监听该socketfd
    ***/
    void listen(); 

    /***
     *  @brief 封装accept函数，调用accept接收新客户连接请求
     *  @param peeraddr 一个InetAddress指针，将accept返回后，客户端的地址存入peeraddr所指地址
     *  @return accept完成后绑定好的文件描述符，该文件描述符表示一个连接。
    ***/
    int accept(InetAddress *peeradd);

    /***
    *	@brief 调用shutdown关闭服务端写通道
    ***/
    void shutdownWrite();  

    //  调用setsockopt来设置一些socket选项  

    void setTcpNoDelay(bool on); //不启用naggle算法，增大对小数据包的支持
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_; //服务器监听套接字文件描述符
};