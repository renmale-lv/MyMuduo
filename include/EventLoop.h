/*
 * @Author: lvxr
 * @Date: 2024-03-03 15:01:53
 * @LastEditTime: 2024-03-04 21:35:28
 */
#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "CurrentThread.h"
#include "noncopyable.h"
#include "TimeStamp.h"

class Channel;
class Poller;

// 事件循环类
class EventLoop : public muduo::noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop(); // 开启事件循环
    void quit(); // 关闭事件循环

    // 返回poller监听到发生事件的时间点
    TimeStamp poolReturnTime() const { return pollReturnTime_; }

    // mainReactor用于唤醒Subreactor的
    void runInLoop(Functor cb);   // mainReactor用于唤醒Subreactor的
    void queueInLoop(Functor cb); 
    
    void wakeup();

    // 更新Channel，在Channel中被调用，Channel将自己加入Loop
    void updateChannel(Channel *channel);

    // 移除某个Channel
    void removeChannel(Channel *channel);

    // 判断是否拥有某个Channel
    bool hasChannel(Channel *channel);

    // 判断当前的eventloop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void handleRead();        // 处理唤醒相关的逻辑。
    void doPendingFunctors(); // 执行回调的

    using ChannelList = std::vector<Channel *>;
    std::atomic<bool> looping_;                // 标志进入loop循环
    std::atomic<bool> quit_;                   // 标志退出loop循环
    std::atomic<bool> callingPendingFunctors_; // 标识当前loop是否有需要执行回调操作
    const pid_t threadId_;                     // 当前loop所在的线程的id
    TimeStamp pollReturnTime_;                 // poller返回发生事件时间点
    std::unique_ptr<Poller> poller_;           // 一个EventLoop需要一个poller，这个poller其实就是操控这个EventLoop的对象

    int wakeupFd_; // 主要作用，当mainLoop获取一个新用户的channel通过轮询算法选择一个subloop(subreactor)来处理channel
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;
    Channel *currentActiveChannel_;
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_;                     // 保护上面vector容器的线程安全操作
};

#endif