/*
 * @Author: lvxr
 * @Date: 2024-03-04 19:54:51
 * @LastEditTime: 2024-03-05 12:59:16
 */
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "Callbacks.h"
#include "TimeStamp.h"
#include "Buffer.h"
#include "InetAddress.h"

class Channel;
class EventLoop;
class Socket;

/**
1. 为什么要用enable_shared_from_this?
    * 需要在类对象的内部中获得一个指向当前对象的shared_ptr 对象
    * 如果在一个程序中，对象内存的生命周期全部由智能指针来管理。在这种情况下，
        要在一个类的成员函数中，对外部返回this指针就成了一个很棘手的问题。
2. 什么时候用？
    * 当一个类被 share_ptr 管理，且在类的成员函数里需要把当前类对象作为参数传给其他函数时，
        这时就需要传递一个指向自身的 share_ptr。
3. 效果：
    TcpConnection类继承 std::enable_shared_from_this ，则会为该类TcpConnection提供成员函数
    shared_from_this。TcpConnection对象 t 被一个为名为 pt 的 std::shared_ptr 类对象管理时，
    调用 T::shared_from_this 成员函数，将会返回一个新的 std::shared_ptr 对象，
    它与 pt 共享 t 的所有权。
*/

// TCP连接类
class TcpConnection : public muduo::noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    /**
     * @description: TcpConnection构造函数
     * @param {EventLoop} *loop 用来处理该Connection的EventLoop
     * @param {string} &name 命名
     * @param {int} sockfd 连接句柄
     * @param {InetAddress} &localAddr 服务器地址
     * @param {InetAddress} &peerAddr 连接地址
     */
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);

    ~TcpConnection();

    // 获取处理该连接的EventLoop
    EventLoop *getLoop() const { return loop_; }

    // 返回本地地址
    const InetAddress &localAddress() const { return localAddr_; }

    // 返回连接地址
    const InetAddress &peerAddress() const { return peerAddr_; }

    // 返回是否连接中
    bool connected() const { return state_ == kConnected; }

    // 向连接写数据
    void send(const std::string &buf);

    // 关闭连接
    void shutdown();

    // 返回连接名字
    const std::string &name() const { return name_; }

    // 设置连接成功回调
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

    // 设置可读回调
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

    // 设置写完成回调
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置关闭回调
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

    // 读写缓冲区超过水位线时的回调
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    // 连接建立
    void connectEstablished();

    // 连接销毁
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected, // 链接
        kConnecting,   // 连接中
        kConnected,    // 没链接
        kDisconnecting // 取消连接中
    };

    // 设置连接状态
    void setState(StateE state) { state_ = state; }
    
    void handleRead(TimeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();

private:
    EventLoop *loop_;        // 这里绝对不是baseLoop，因为TcpConnection都是在subLoop里面管理的
    const std::string name_; // 连接名字
    std::atomic<int> state_; // 连接状态
    bool reading_;

    std::unique_ptr<Socket> socket_;   // 连接句柄
    std::unique_ptr<Channel> channel_; // 连接对应的Channel

    const InetAddress localAddr_; // 本地地址
    const InetAddress peerAddr_;  // 连接地址

    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完后的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 发送和接收缓冲区超过水位线的话容易导致溢出，这是很危险的。
    CloseCallback closeCallback_;                 // 关闭回调
    size_t highWaterMark_;                        // 水位线
    Buffer inputBuffer_;                          // 接收的缓冲区
    Buffer outputBuffer_;                         // 发送的缓冲区
};

#endif