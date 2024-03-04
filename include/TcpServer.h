/*
 * @Author: lvxr
 * @Date: 2024-03-04 19:54:57
 * @LastEditTime: 2024-03-04 21:10:56
 */
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

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

// TCP服务器类
class TcpServer : public muduo::noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    // 预置两个选项，是否对端口进行复用
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    /**
     * @description: TcpServer构造函数
     * @param {EventLoop} *loop  底层线程池baseloop，由用户自己创建
     * @param {InetAddress} &listenAddr 监听的网络地址
     * @param {string} &nameArg  底层线程池名字
     * @param {Option} option 是否重用端口，默认不重用
     */
    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);

    ~TcpServer();

    // 设置loop线程初始化的回调
    void setThreadInitcallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }

    // 设置新连接回调
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

    // 设置有读写消息时的回调
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

    // 设置消息发送完成的回调
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subloop个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

private:
    // 处理新连接到来
    void newConnection(int sockfd, const InetAddress &peerAddr);

    // 移除连接，TCP连接Close自动调用的回调函数
    void removeConnection(const TcpConnectionPtr &conn);

    // 移除连接，不直接使用，被removeConnection函数内部调用
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    EventLoop *loop_;                                 // baseLoop，用户自己定义的
    const std::string ipPort_;                        // 服务器监听地址
    const std::string name_;                          // 服务器名字
    std::unique_ptr<Acceptor> acceptor_;              // 运行在baseLoop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 底层线程池
    ConnectionCallback connectionCallback_;           // 有新连接时的回调
    MessageCallback messageCallback_;                 // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;     // 消息发送完成的回调
    ThreadInitCallback threadInitCallback_;           // loop线程初始化的回调
    std::atomic<int> started_;                        // 服务器是否启动，大于等于0时为启动
    int nextConnId_;                                  // 下一个新连接的id，用于计数
    ConnectionMap connections_;                       // 保存所有的连接
};

#endif