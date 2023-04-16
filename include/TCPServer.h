#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include <functional>
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include <string>
#include <atomic>
#include <memory>
#include <unordered_map>



/***
 *  @brief 一个TCP服务器类，可创建一个TCP服务器对象
 ***/
class TcpServer : noncopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    //是否对端口进行复用
    enum Option{
        kNoReusePort,
        kReusePort,
    };

    /***
     *  @brief 一个TCP服务器类，可创建一个TCP服务器对象
     *  @param loop 绑定的的EventLoop，该EventLoop作为主reactor，负责监听连接
     *  @param listenAddr 该服务器被监听的socket地址
     *  @param nameArg 给服务器取个好听的名字吧！
     *  @param option  选项，默认为kNoReusePort，即监听端口不可复用
     ***/
    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option = kNoReusePort);


    /***
     *  @brief 析构函数
     ***/
    ~TcpServer();

    /***
     *  @brief 
     *  @param 
     ***/
    const std::string& ipPort() const { return ipPort_; }

    /***
     *  @brief 
     *  @param 
     ***/
    const std::string& name() const { return name_; }

    /***
     *  @brief 
     *  @return 
     ***/
    EventLoop* getLoop() const { return loop_; }

    /***
     *  @brief 
     *  @param 
     ***/
    void setThreadNum(int numThreads);


    /***
     *  @brief 
     *  @param 
     ***/
    void setThreadInitCallback(const ThreadInitCallback& cb){
        threadInitCallback_ = cb;
    }

    /***
     *  @brief 
     *  @return 
     ***/
    std::shared_ptr<EventLoopThreadPool> threadPool(){
        return threadPool_;
    }


    /***
     *  @brief 开启监听，开启多次产生未定义行为，线程安全
     ***/
    void start();

    // 设置连接后的回调函数
    // 线程不安全.
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }

    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }

private:

    /***
     *  @brief 新连接到来时的处理函数
     *  @param sockfd 与新连接绑定的文件描述符
     *  @param peerAddr 用于新连接中客户端的地址存放
     ***/
    void newConnection(int sockfd, const InetAddress& peerAddr);


    /// Thread safe.
    void removeConnection(const TcpConnectionPtr& conn);
    /// Not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::unordered_map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;  // the acceptor loop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    std::atomic<int> started_;
    // always in loop thread
    int nextConnId_;
    ConnectionMap connections_;
};