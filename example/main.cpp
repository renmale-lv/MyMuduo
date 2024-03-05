/*
 * @Author: lvxr
 * @Date: 2024-03-05 14:33:10
 * @LastEditTime: 2024-03-05 14:56:05
 */

#include <EventLoop.h>
#include <InetAddress.h>
#include <TcpServer.h>
#include <TcpConnection.h>
#include <Logger.h>
#include <TimeStamp.h>
#include <Buffer.h>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name),
          loop_(loop)
    {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        else
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, TimeStamp time)
    {
        std::string msg = buf->retrieveAllString();
        conn->send(msg);
        conn->shutdown();
    }

private:
    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(9993);
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    // baseloop需要自己手动loop
    loop.loop();
    return 0;
}