#pragma once
#include "noncopyable.h"
#include "InetAddress.h"


/***
 *	@brief �����װ��һ���ļ�������sockfd��Ӧ���ڸ��ļ��������ķ���
 ***/
class Socket : public noncopyable{
public:
    /***
    *	@brief ���캯����ͨ������sockfd����һ��Socket����
    *   @param sockfd ���ڹ���Socket�����sockfd
    ***/
    explicit Socket(int sockfd) : sockfd_(sockfd){}


    /***
    *	@brief ��������
    ***/
    ~Socket();

public:


    /***
    *	@brief ��ȡ��Socket�����װ��sockfd
    *   @return ��Socket�����װ��sockfd
    ***/
    int fd() const {return sockfd_;}


    /***
    *	@brief ��װbind������bind���ڲ�sockfd��ָ����ַ���а�
    *   @param localaddr ��Ҫ�󶨵�sockfd�ϵĵ�ַ
    ***/
    void bindAddress(const InetAddress &localaddr); //����bind�󶨷�����IP�˿�

    /***
    *	@brief ��װlisten������listen������socketfd
    ***/
    void listen(); 

    /***
     *  @brief ��װaccept����������accept�����¿ͻ���������
     *  @param peeraddr һ��InetAddressָ�룬��accept���غ󣬿ͻ��˵ĵ�ַ����peeraddr��ָ��ַ
     *  @return accept��ɺ�󶨺õ��ļ������������ļ���������ʾһ�����ӡ�
    ***/
    int accept(InetAddress *peeradd);

    /***
    *	@brief ����shutdown�رշ����дͨ��
    ***/
    void shutdownWrite();  

    //  ����setsockopt������һЩsocketѡ��  

    void setTcpNoDelay(bool on); //������naggle�㷨�������С���ݰ���֧��
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_; //�����������׽����ļ�������
};