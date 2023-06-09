#pragma once
#include <memory>
#include "noncopyable.h"
#include <string>
#include <atomic>
#include "InetAddress.h"
#include "Callbacks.h"
#include "TimeStamp.h"
#include "Buffer.h"
class Channel;
class EventLoop;
class Socket;
class Buffer;

struct tcp_info;

class Channel;
class EventLoop;
class Socket;

//什么是enable_shared_from_this?
//即继承自enable_shared_from_this的类，类内部的成员函数可以通过调用shared_from_this获得一个管理this指针的shared_ptr
//这样是安全的，因为如果直接返回this，外界是不知道this何时失效的

/***
 *  @brief 该类封装了一个连接成功的完整的TCP连接
***/
class TcpConnection: public noncopyable, public std::enable_shared_from_this<TcpConnection>{
public:

    /***
     *  @brief 构造函数，构造函数中会给其绑定的channel注册相应的回调函数
     *  @param loop 该TcpConnection所属的EventLoop
     *  @param name 取个名字吧
     *  @param sockfd 该连接的sockfd
     *  @param locakAddr 服务器地址
     *  @param peerAddr 客户端地址
    ***/
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    /***
     * @brief 析构函数
    ***/  
    ~TcpConnection();

    /***
     * @brief 获得该TcpConnection所属的EventLoop
     * @return 所属EventLoop
    ***/ 
    EventLoop* getLoop() const { return loop_; }

    /***
     * @brief 获得该TcpConnection的名字
     * @return name
    ***/ 
    const std::string& name() const { return name_; }  

    /***
     * @brief 获得该TcpConnection中服务器端的地址
     * @return address
    ***/ 
    const InetAddress& localAddress() const { return localAddr_; }

    /***
     * @brief 获得该TcpConnection中客户端的地址
     * @return address
    ***/ 
    const InetAddress& peerAddress() const { return peerAddr_; }

    /***
     * @brief 该TcpConnection的状态是否为已连接
     * @return bool
    ***/ 
    bool connected() const { return state_ == kConnected; }
    
    /***
     * @brief 该TcpConnection的状态是否为断开
     * @return bool
    ***/ 
    bool disconnected() const { return state_ == kDisconnected; }


    
    // return true if success.
    bool getTcpInfo(struct tcp_info*) const;
    std::string getTcpInfoString() const;

    void send(std::string&& message); // C++11

    void send(Buffer* message);  // this one will swap data
    void shutdown(); // NOT thread safe, no simultaneous calling
    // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
    void forceClose();
    void forceCloseWithDelay(double seconds);
    void setTcpNoDelay(bool on);
    // reading or not
    void startRead();
    void stopRead();
    bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    /// Advanced interface
    Buffer* inputBuffer()
    { return &inputBuffer_; }

    Buffer* outputBuffer()
    { return &outputBuffer_; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback& cb)
  { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  
  void handleRead(TimeStamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  void sendInLoop(std::string&& message);

  void sendInLoop(const void* message, size_t len);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop* loop_;
  const std::string name_;
  StateE state_;  // FIXME: use atomic variable
  bool reading_;
  // we don't expose those classes to client.
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  size_t highWaterMark_;
  Buffer inputBuffer_;
  Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.

};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;