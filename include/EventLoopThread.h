/*
 * @Author: lvxr
 * @Date: 2024-03-04 14:44:08
 * @LastEditTime: 2024-03-04 15:57:44
 */
#ifndef EVENTLOOP_THREAD_H
#define EVENTLOOP_THREAD_H

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread : public muduo::noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());
    ~EventLoopThread();

    // 启动子线程，并返回loop
    EventLoop *startLoop();

private:
    void threadFunc();

private:
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};

#endif