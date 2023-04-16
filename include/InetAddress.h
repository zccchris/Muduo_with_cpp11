#pragma once

#include <arpa/inet.h> 
#include <netinet/in.h> //sockaddr_in
#include <string>

/***
 *  @brief 封装socket地址，这里只对ipv4进行封装，即地址族协议为AF_INET
 *         其实就是封装了一个sockaddr_in
 */
class InetAddress{
public:
    /***
    *   @brief 默认构造函数，默认为127.0.0.1:0
    *   @param port 端口号
    *   @param ip ip地址
    ***/
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");


    /***
    *   @brief 根据sockaddr_in构造
    *   @param addr sockaddr_in类型地址
    ***/
    explicit InetAddress(const sockaddr_in &addr);


    /***
    *   @brief 获取string类型的该地址对象的ip地址
    *   @return string类型，(点分十进制)ip地址
    ***/
    std::string toIp() const;

    /***
    *   @brief 获取string类型的该地址对象的ip地址及端口
    *   @return string类型，(点分十进制)ip地址:端口
    ***/
    std::string toIpPort() const;

    /***
    *   @brief 获取string类型的该地址对象的端口
    *   @return string类型，端口号
    ***/
    uint16_t toPort() const;
    
    /***
    *   @brief 获取地址
    *   @return sockaddr_in类型地址
    ***/
    const sockaddr_in *getSockAddr() const;


    /***
    *   @brief 设置Socket地址
    *   @param addr sockaddr_in类型地址
    ***/
    void setSockAddr(const sockaddr_in &addr){addr_ = addr;}
private:
    sockaddr_in addr_;
};