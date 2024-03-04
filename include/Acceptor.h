/*
 * @Author: lvxr
 * @Date: 2024-03-04 17:03:39
 * @LastEditTime: 2024-03-04 17:07:15
 */
#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <functional>

#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

class Acceptor : public muduo::noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // 设置连接事件发生的回调函数,当有连接事件发生的时候调用newConnectionCallback_
    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

    // 是否正在监听
    bool listenning() const { return listenning_; }

    // 开始监听
    void listen();

private:
    void handleRead();

    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_; 
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};

#endif