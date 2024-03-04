/*
 * @Author: lvxr
 * @Date: 2024-03-04 15:58:13
 * @LastEditTime: 2024-03-04 21:00:48
 */

#ifndef EVENTLOOP_THREADPOOL_H
#define EVENTLOOP_THREADPOOL_H

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

// 管理EventLoopThread的线程池
class EventLoopThreadPool : public muduo::noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 轮询算法，获取下一个空闲的loop
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllGroups();
    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    // 如果你没有通过setThreadNum来设置线程数量，那整个网络框架就只有一个线程，这唯一的一个线程就是这个baseLoop_，既要处理新连接，还要处理已连接的事件监听
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;   // 是否启动
    int numThreads_; // subloop数量
    int next_;       // 轮询游标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_; // 包含了所有EventLoop线程的指针
};

#endif