/*
 * @Author: lvxr
 * @Date: 2024-03-04 13:04:45
 * @LastEditTime: 2024-03-04 13:30:36
 */
#ifndef THREAD_H
#define THREAD_H

#include <functional>
#include <thread>
#include <memory>
#include <atomic>

#include "noncopyable.h"

// 自定义线程类
class Thread : public muduo::noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();                             // 启动线程
    void join();                              // join线程
    bool started() const { return started_; } // 返回线程是否启动

private:
    // 如果构造函数没有传入名字，则赋予线程一个默认的名字，并更新numCreated_变量，用作初始化
    void setDefaultName();

private:
    bool started_;     // 线程是否启动
    bool joined_;      // 线程是否join
    pid_t tid_;        // 线程pid
    ThreadFunc func_;  // 线程运行的主函数
    std::string name_; // 线程名字

    // 注意这里，如果你直接定义一个thread对象，那这个线程就直接开始运行了，所以这里定义一个智能指针，在需要运行的时候再给他创建对象
    std::shared_ptr<std::thread> thread_;

    // 静态原子变量，记录一共创建了多少个线程
    static std::atomic<int> numCreated_;
};

#endif