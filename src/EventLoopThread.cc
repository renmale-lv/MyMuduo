/*
 * @Author: lvxr
 * @Date: 2024-03-04 14:44:16
 * @LastEditTime: 2024-03-04 15:50:03
 */

#include "EventLoopThread.h"

#include <memory>

#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb) {}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        // 等待线程结束
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    // 启动线程
    thread_.start();
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            // 放在循环里面防止假唤醒，明明自己被唤醒了，刚想要得到资源的时候却被别人剥夺了
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    if (callback_)
        callback_(&loop);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();                               // 开始监听
    std::unique_lock<std::mutex> lock(mutex_); // 悲观锁
    loop_ = nullptr;                           // 如果走到这里说明服务器程序要关闭了，不进行事件循环了
}