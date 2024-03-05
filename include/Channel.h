/*
 * @Author: lvxr
 * @Date: 2024-03-02 15:59:21
 * @LastEditTime: 2024-03-05 13:49:20
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "TimeStamp.h"

class EventLoop;

// Channel封装了sockfd和它感兴趣的event，还绑定了poller返回的具体事件
class Channel : public muduo::noncopyable
{
public:
    // 事件回调
    using EventCallback = std::function<void()>;

    // 只读事件回调
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void HandlerEvent(TimeStamp receive_time);

    // 设置可读事件回调函数
    void setReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }

    // 设置可写事件回调函数
    void setWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }

    // 设置关闭事件回调函数
    void setCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }

    // 设置错误事件回调函数
    void setErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }

    void tie(const std::shared_ptr<void> &);

    // 返回封装的fd
    int fd() const { return fd_; }

    // 返回正在监听的事件
    int events() const { return events_; }

    // 设置发生的事件
    void set_revents(int revt) { revents_ = revt; }

    // 监听可读事件
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }

    // 取消监听可读事件
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }

    // 监听可写事件
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }

    // 取消监听可写事件
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    // 取消监听的全部事件
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 当前监听的事件是否包含可写事件
    bool isWriting() const { return events_ & kWriteEvent; }

    // 当前监听的事件是否包含可读事件
    bool isReading() const { return events_ & kReadEvent; }

    // 当前是否有监听事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    // 返回当前状态
    int status() { return status_; }

    // 设置当前状态
    void set_status(int status) { status_ = status; }

    // 返回属于哪个EventLoop对象
    EventLoop *ownerLoop() { return loop_; }

    // 从所属的EventLoop中删除该Channel
    void remove();

private:
    // 更新fd_的监听事件
    void update();

    // 根据revents_互调具体的回调函数
    void HandleEventWithGuard(TimeStamp receiveTime);

    static const int kNoneEvent;  // 空事件
    static const int kReadEvent;  // 可读事件
    static const int kWriteEvent; // 可写事件

    EventLoop *loop_; // 记录这个channel属于哪个EventLoop对象

    const int fd_; // 封装的socket
    int events_;   // socket要监听的事件，EPOLLIN | EPOLLPRI，EPOLLPRI：带外数据
    int revents_;  // socket发生的事件
    int status_;   // channel的状态，在EpollPoller中定位了各状态

    std::weak_ptr<void> tie_; // 用来绑定一个连接，避免连接释放后继续执行回调函数，具体使用在HandlerEvent函数中
    bool tied_;               // 是否绑定了tie_

    // 回调函数
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

#endif