#pragma once

#include <functional>
#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

/***
*	����һ��acceptor�������acceptor����ֻ�������һ�������úõ�socket��ַ������������㻹��Ҫ
*	����һ�������õ�EventLoop�������acceptor�����EventLoop�󶨣��󶨺ú��EventLoopΪ��reactor��
*	Ȼ����Ҫ����Acceptor�ĳ�Ա����listen����Ӧ�˿ڽ��м����������Ѷ���÷����ɶ��¼�(�пͻ��˷�����������)�Ĵ���������accept
*	�����ǻ���Ҫ���������Ӳ�����Ļص���������muduo�У��ú����������ǽ������ӷ�����µ�EventLoop
***/

class Acceptor : noncopyable{
public:
	typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;
/***
*	����һ��EventLoop
*	����һ��listenAddr
***/
	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
	~Acceptor();

	//���������¼������Ļص����������������µ����ӣ�Ӧ����ʲô
	void setNewConnectionCallback(const NewConnectionCallback& cb){
		newConnectionCallback_ = cb;
	}

	void listen();

	bool listening() const { return listening_; }

private:
	void handleRead();

	EventLoop* loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;	//�Ƿ��ڼ���״̬
};