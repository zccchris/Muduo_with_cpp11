#pragma once
#include "noncopyable.h"
#include "InetAddress.h"

class Socket : public noncopyable{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd){}
    ~Socket();

public:
    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress &localaddr); //����bind�󶨷�����IP�˿�
    void listen(); //����listen�����׽���
    int accept(InetAddress *peeradd); //����accept�����¿ͻ���������
    void shutdownWrite();  //����shutdown�رշ����дͨ��

    /**  ����setsockopt������һЩsocketѡ��  **/
    void setTcpNoDelay(bool on); //������naggle�㷨�������С���ݰ���֧��
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_; //�����������׽����ļ�������
};