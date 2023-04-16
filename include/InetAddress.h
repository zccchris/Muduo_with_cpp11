#pragma once

#include <arpa/inet.h> 
#include <netinet/in.h> //sockaddr_in
#include <string>

/***
 *  @brief ��װsocket��ַ������ֻ��ipv4���з�װ������ַ��Э��ΪAF_INET
 *         ��ʵ���Ƿ�װ��һ��sockaddr_in
 */
class InetAddress{
public:
    /***
    *   @brief Ĭ�Ϲ��캯����Ĭ��Ϊ127.0.0.1:0
    *   @param port �˿ں�
    *   @param ip ip��ַ
    ***/
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");


    /***
    *   @brief ����sockaddr_in����
    *   @param addr sockaddr_in���͵�ַ
    ***/
    explicit InetAddress(const sockaddr_in &addr);


    /***
    *   @brief ��ȡstring���͵ĸõ�ַ�����ip��ַ
    *   @return string���ͣ�(���ʮ����)ip��ַ
    ***/
    std::string toIp() const;

    /***
    *   @brief ��ȡstring���͵ĸõ�ַ�����ip��ַ���˿�
    *   @return string���ͣ�(���ʮ����)ip��ַ:�˿�
    ***/
    std::string toIpPort() const;

    /***
    *   @brief ��ȡstring���͵ĸõ�ַ����Ķ˿�
    *   @return string���ͣ��˿ں�
    ***/
    uint16_t toPort() const;
    
    /***
    *   @brief ��ȡ��ַ
    *   @return sockaddr_in���͵�ַ
    ***/
    const sockaddr_in *getSockAddr() const;


    /***
    *   @brief ����Socket��ַ
    *   @param addr sockaddr_in���͵�ַ
    ***/
    void setSockAddr(const sockaddr_in &addr){addr_ = addr;}
private:
    sockaddr_in addr_;
};