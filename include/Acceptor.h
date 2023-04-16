#pragma once

#include <functional>
#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

/***
*	@brief ����һ��acceptor�������acceptor����ֻ�������һ�������úõ�socket��ַ������������㻹��Ҫ
*		   ����һ�������õ�EventLoop�������acceptor�����EventLoop�󶨣��󶨺ú��EventLoopΪ��reactor��
*	       Ȼ����Ҫ����Acceptor�ĳ�Ա����listen����Ӧ�˿ڽ��м����������Ѷ���÷����ɶ��¼�(�пͻ��˷�����������)�Ĵ���������accept
*	       �����ǻ���Ҫ���������Ӳ�����Ļص���������muduo�У��ú����������ǽ������ӷ�����µ�EventLoop
***/
class Acceptor : noncopyable{
public:
	typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

	/***
	 *	@brief ����һ��Acceptor
	 *	@param loop ��Acceptor��Ҫע�ᵽloop�У���loop����Ϊ��reactor
	 *	@param listenAddr ��Acceptor�����ĵ�ַ
	 *  @param reuseport �˿��Ƿ�ɸ���
	 ***/
	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);

	/***
	 *	@brief Ĭ���������� 
	 ***/
	~Acceptor();

	/***
	 *	@brief ���������¼������Ļص��������������µ����ӣ����ûص�����Ϊcb
	 *  @param cb void()(int sockfd, const InetAddress&)���ͺ���
	 ***/
	void setNewConnectionCallback(const NewConnectionCallback& cb) {newConnectionCallback_ = cb;}

	
	/***
	 *	@brief ��ʼ���� 
	 ***/
	void listen();

	/***
	 *	@brief �Ƿ��ڼ���״̬
	 *	@return ��/�� 
	 ***/
	bool listening() const { return listening_; }

private:
	/***
 	*	@brief �пɶ��¼��������������û������ˣ���ʱ���øú���
	*		   �����е������Ȱ󶨺õĻص�����
	*		   �������Ӳ�����sockfd��ͻ��˵ĵ�ַ��Ϊ�ص������Ĳ���
 	***/
	void handleRead();

	EventLoop* loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;	//�Ƿ��ڼ���״̬
};