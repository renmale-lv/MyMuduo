/*
 * @Author: lvxr
 * @Date: 2024-03-04 16:55:54
 * @LastEditTime: 2024-03-05 13:34:04
 */
#ifndef SOCKET_H
#define SOCKET_H

#include "noncopyable.h"

class InetAddress;

// socket封装类
class Socket : public muduo::noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

public:
    int fd() const { return sockfd_; }              // 获取文件描述符
    void bindAddress(const InetAddress &localaddr); // 调用bind绑定服务器IP端口
    void listen();                                  // 调用listen监听套接字
    int accept(InetAddress *peeradd);               // 调用accept接受新客户连接请求
    void shutdownWrite();                           // 调用shutdown关闭服务端写通道

    void setTcpNoDelay(bool on); // 不启用naggle算法，增大对小数据包的支持
    void setReuseAddr(bool on);  // 重用本地地址
    void setReusePort(bool on);  // 重用端口
    void setKeepAlive(bool on);  // 发送周期性保活报文以维持连接

private:
    const int sockfd_;
}

#endif