#pragma once

#include <functional>
#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

/***
*	@brief 创建一个acceptor对象，这个acceptor对象只负责接受一个你设置好的socket地址，这个过程中你还需要
*		   传入一个声明好的EventLoop，将这个acceptor与这个EventLoop绑定，绑定好后该EventLoop为主reactor。
*	       然后需要调用Acceptor的成员函数listen对相应端口进行监听，类中已定义好发生可读事件(有客户端发起连接请求)的处理函数，即accept
*	       但我们还需要设置新连接产生后的回调函数，在muduo中，该函数的作用是将新连接分配给新的EventLoop
***/
class Acceptor : noncopyable{
public:
	typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

	/***
	 *	@brief 构造一个Acceptor
	 *	@param loop 该Acceptor需要注册到loop中，该loop将成为主reactor
	 *	@param listenAddr 该Acceptor监听的地址
	 *  @param reuseport 端口是否可复用
	 ***/
	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);

	/***
	 *	@brief 默认析构函数 
	 ***/
	~Acceptor();

	/***
	 *	@brief 设置连接事件发生的回调函数，发生了新的连接，设置回调函数为cb
	 *  @param cb void()(int sockfd, const InetAddress&)类型函数
	 ***/
	void setNewConnectionCallback(const NewConnectionCallback& cb) {newConnectionCallback_ = cb;}

	
	/***
	 *	@brief 开始监听 
	 ***/
	void listen();

	/***
	 *	@brief 是否处于监听状态
	 *	@return 是/否 
	 ***/
	bool listening() const { return listening_; }

private:
	/***
 	*	@brief 有可读事件发生，即有新用户连接了，此时调用该函数
	*		   函数中调用事先绑定好的回调函数
	*		   将新连接产生的sockfd与客户端的地址作为回调函数的参数
 	***/
	void handleRead();

	EventLoop* loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;	//是否处于监听状态
};