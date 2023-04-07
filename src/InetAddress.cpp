#include "InetAddress.h"
#include <cstring>
#include <string>


InetAddress::InetAddress(const sockaddr_in &addr)
    : addr_(addr) {
}

InetAddress::InetAddress(uint16_t port, std::string ip){
    bzero(&addr_, sizeof(addr_)); //man bzero ͷ�ļ���strings.h
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); //�������ֽ�˳��ת��������ֽ�˳��
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); //inet_addr()����������cp ��ָ�������ַ�ַ���ת����������ʹ�õĶ���������
}

 //����ֵ��ʽת��Ϊ���ʮ���Ƶ��ַ���ip��ַ��ʽ
std::string InetAddress::toIp() const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
std::string InetAddress::toIpPort() const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf); 
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

//��һ���޷��Ŷ��������������ֽ�˳��ת��Ϊ�����ֽ�˳��
uint16_t InetAddress::toPort() const{
    //
    return ntohs(addr_.sin_port);  
}