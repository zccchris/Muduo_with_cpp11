#pragma once

#include <functional>
#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

/***
*	创建一个acceptor对象，这个acceptor对象只负责接受一个你设置好的socket地址，这个过程中你还需要
*	传入一个声明好的EventLoop，将这个acceptor与这个EventLoop绑定，绑定好后该EventLoop为主reactor。
*	然后需要调用Acceptor的成员函数listen对相应端口进行监听，类中已定义好发生可读事件(有客户端发起连接请求)的处理函数，即accept
*	但我们还需要设置新连接产生后的回调函数，在muduo中，该函数的作用是将新连接分配给新的EventLoop
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
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;	//是否处于监听状态
};