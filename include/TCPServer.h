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

class TcpServer : noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    //是否对端口进行复用
    enum Option{
        kNoReusePort,
        kReusePort,
    };


    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option = kNoReusePort);
    ~TcpServer();

    const std::string& ipPort() const { return ipPort_; }
    const std::string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }


    void setThreadNum(int numThreads);

    void setThreadInitCallback(const ThreadInitCallback& cb){
        threadInitCallback_ = cb;
    }


    std::shared_ptr<EventLoopThreadPool> threadPool(){
        return threadPool_;
    }

    
    // 线程安全
    // 开启监听，开启多次产生未定义行为
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
    /// Not thread safe, but in loop
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