/*
 * @Author: lvxr
 * @Date: 2024-03-04 13:04:51
 * @LastEditTime: 2024-03-05 14:02:21
 */
#include "Thread.h"

#include <semaphore.h>

#include "CurrentThread.h"

std::atomic<int> Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    // 分离线程
    if (started_ && !joined_)
        thread_->detach();
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    // 初始化信号量为0
    sem_init(&sem, false, 0);
    // start时才实例化线程
    thread_ = std::shared_ptr<std::thread>(new std::thread(
        [&]()
        {
            tid_=CurrentThread::tid();
            sem_post(&sem);
            func_(); }));
    // 阻塞只要线程开始执行func_函数，目的时为了获取线程tid
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num); /// 给这个线程一个名字
    }
}