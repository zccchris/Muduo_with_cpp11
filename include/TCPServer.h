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
 *  @brief һ��TCP�������࣬�ɴ���һ��TCP����������
 ***/
class TcpServer : noncopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    //�Ƿ�Զ˿ڽ��и���
    enum Option{
        kNoReusePort,
        kReusePort,
    };

    /***
     *  @brief һ��TCP�������࣬�ɴ���һ��TCP����������
     *  @param loop �󶨵ĵ�EventLoop����EventLoop��Ϊ��reactor�������������
     *  @param listenAddr �÷�������������socket��ַ
     *  @param nameArg ��������ȡ�����������ְɣ�
     *  @param option  ѡ�Ĭ��ΪkNoReusePort���������˿ڲ��ɸ���
     ***/
    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option = kNoReusePort);


    /***
     *  @brief ��������
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
     *  @brief ����������������β���δ������Ϊ���̰߳�ȫ
     ***/
    void start();

    // �������Ӻ�Ļص�����
    // �̲߳���ȫ.
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
     *  @brief �����ӵ���ʱ�Ĵ�����
     *  @param sockfd �������Ӱ󶨵��ļ�������
     *  @param peerAddr �����������пͻ��˵ĵ�ַ���
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