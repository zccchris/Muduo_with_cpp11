#include "InetAddress.h"
#include <cstring>
#include <string>


InetAddress::InetAddress(const sockaddr_in &addr)
    : addr_(addr) {
}

InetAddress::InetAddress(uint16_t port, std::string ip){
    bzero(&addr_, sizeof(addr_)); //man bzero 头文件在strings.h
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); //从主机字节顺序转变成网络字节顺序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); //inet_addr()用来将参数cp 所指的网络地址字符串转换成网络所使用的二进制数字
}


std::string InetAddress::toIp() const{
    char buf[64] = {0};
     //将数值格式转化为点分十进制的字符串ip地址格式
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}


std::string InetAddress::toIpPort() const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); // Internet网络地址转换为标准格式字符串
    size_t end = strlen(buf); 
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}


uint16_t InetAddress::toPort() const{
    //将一个无符号短整形数从网络字节顺序转换为主机字节顺序
    return ntohs(addr_.sin_port);  
}