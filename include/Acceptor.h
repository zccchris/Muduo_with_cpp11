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
*	接收一个EventLoop
*	接收一个listenAddr
***/
	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
	~Acceptor();

	//设置连接事件发生的回调函数，即发生了新的连接，应该做什么
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