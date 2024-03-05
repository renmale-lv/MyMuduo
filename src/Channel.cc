/*
 * @Author: lvxr
 * @Date: 2024-03-02 17:07:12
 * @LastEditTime: 2024-03-05 13:51:30
 */

#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), status_(-1), tied_(false) {}

Channel::~Channel() {}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    /**
     * @brief channel的tie方法什么时候调用？一个TcpConnection新连接创建的时候，
     * TcpConnection => Channel;
     */
    tie_ = obj;
    tied_ = true;
}

void Channel::update()
{
    /**
     * 当改变channel所对应的fd的events事件后，update负责在poller里面更改
     * fd相应的事件epoll_ctl
     * 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
     * channel和poller通过EventLoop进行连接。
     */
    loop_->updateChannel(this);
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::HandlerEvent(TimeStamp receiveTime)
{
    if (tied_)
    {
        // lock返回一个weak_ptr指向相同的shared_ptr，如果已经被释放则返回null
        // tie_绑定一个TCPConnection，用于避免连接被释放后回调函数的执行
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
            HandleEventWithGuard(receiveTime);
    }
    else
        HandleEventWithGuard(receiveTime);
}

void Channel::HandleEventWithGuard(TimeStamp receiveTime)
{
    // 打印日志
    LOG_INFO("channel HandleEvent revents:%d", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        // 设备断开连接且无数据可读
        if (close_callback_)
            close_callback_();
    }
    if (revents_ & EPOLLERR)
    {
        // 发生错误事件
        if (error_callback_)
            error_callback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        // 有数据可读
        if (read_callback_)
            read_callback_(receiveTime);
    }
    if (revents_ & EPOLLOUT)
    {
        // 有数据可写
        if (write_callback_)
            write_callback_();
    }
}
