#pragma once

#include <functional>
#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

/***
*	
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
	int acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;
};