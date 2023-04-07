#pragma once

#include <arpa/inet.h> //
#include <netinet/in.h> //sockaddr_in
#include <string>

/***
 封装socket地址类型，这里只对ipv4进行封装，即地址族协议为AF_INET
 其实就是封装了一个sockaddr_in
 */
class InetAddress{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr);
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    
    const sockaddr_in *getSockAddr() const;
    void setSockAddr(const sockaddr_in &addr){addr_ = addr;}
private:
    sockaddr_in addr_;
};