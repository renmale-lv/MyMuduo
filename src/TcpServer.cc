/*
 * @Author: lvxr
 * @Date: 2024-03-04 19:55:12
 * @LastEditTime: 2024-03-04 21:10:17
 */
#include "TcpServer.h"
#include "Logger.h"
#include <strings.h>
#include "TcpConnection.h"

using namespace std;
using namespace std::placeholders;

// 这里定义为static怕TcpConnection和TcpServer的这个函数产生冲突
// 检查EventLoop是否为空
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
        LOG_FATAL("%s:%s:%d mainLoop is null \b", __FILE__, __FUNCTION__, __LINE__);
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg, Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    acceptor_->setNewConnectionCallback(bind(&TcpServer::newConnection, this, _1, _2)); // 这两个占位符是connfd和ip地址端口号
}

TcpServer::~TcpServer()
{
    // connections类型为std::unordered_map<std::string, TcpConnectionPtr>;
    for (auto &item : connections_)
    {
        /***
         注意这里的写法，先让conn持有这个item.second这个智能指针
         然后把item.second 这个智能指针释放了，但是此时item.second所指向的资源
         其实还没有释放，因为这个资源现在还被conn管理，不过当这个for循环结束后，这个
         conn离开作用域，这个智能指针也会被释放。

         为什么要这么麻烦，我个人推测，不过逻辑还没闭环： 因为我们后面还要调用TcpConnection的connectDestroyed方法
         而且这个方法的调用还是在另一个线程中执行的，还是担心悬空指针的问题。
         ***/
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() // 开启服务器监听
{
    // 防止一个TcpServer对象被start多次
    if (started_++ == 0)
    {
        threadPool_->start(threadInitCallback_); // 启动底层的loop线程池
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
        // 让这个EventLoop，也就是mainloop来执行Acceptor的listen函数，开启服务端监听
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = threadPool_->getNextLoop(); // 轮循算法选择一个subLoop来管理新连接的channel
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_); // 表示一个连接的名称
    ++nextConnId_;                                                      // 没必要把这个变量变为原子变量，因为这个nextConnId也只会在baseloop里面处理
    string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    { // getsockname函数用于获取与某个套接字关联的本地协议地址，然后放到local里面
        LOG_ERROR("sockets::getLocalAddr");
    }

    InetAddress localAddr(local);
    // 根据连接成功的sockfd创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;

    // 下面的回调都是用户设置给TcpServer的
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(bind(&TcpServer::removeConnection, this, _1));

    ioLoop->runInLoop(bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    // 当TcpConnection的CloseCallback调用的回调函数
    loop_->runInLoop(
        bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(bind(&TcpConnection::connectDestroyed, conn));
    // 拐来拐去最后又拐到connectDestroyed
}
