/*
 * @Author: lvxr
 * @Date: 2024-03-03 14:09:14
 * @LastEditTime: 2024-03-05 13:52:58
 */
#include "EpollPoller.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "Logger.h"
#include "Channel.h"

/* 下面三个常量值表示了一个channel的三种状态 */
// channel未添加到poller中
const int kNew = -1; // channel的成员index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    // epoll_create创建失败则fatal error退出
    if (epollfd_ < 0)
        LOG_FATAL("epoll create error:%d \n", errno);
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

void EpollPoller::updateChannel(Channel *channel)
{
    // 获取当前channel的状态，刚创建还是已在EventLoop上注册还是已在EventLoop删除
    const int status = channel->status();
    LOG_INFO("fd=%d events=%d status=%d \n", channel->fd(), channel->events(), status);
    if (status == kNew || status == kDeleted)
    {
        // 这个channel从来都没有添加到poller中，那么就添加到poller的channel_map中
        if (status == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        // 设置当前channel的状态
        channel->set_status(kAdded);
        // 注册新的fd到epfd中；
        update(EPOLL_CTL_ADD, channel);
    }
    // channel已经在poller上注册过了
    else
    {
        int fd = channel->fd();
        // 这个channel已经对任何事件都不感兴趣了
        if (channel->isNoneEvent())
        {
            // 从epfd中删除一个fd；
            update(EPOLL_CTL_DEL, channel);
            channel->set_status(kDeleted);
        }
        else
        {
            // 这个channel还是对某些事件感兴趣的，修改已经注册的fd的监听事件；
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    // 这个__FUNCTION__是获取函数名
    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int status = channel->status();
    if (status == kAdded)
        update(EPOLL_CTL_DEL, channel);

    channel->set_status(kNew);
}

// 填写活跃的连接
void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        // 设置发生的事件，通知channel事件发生了
        channel->set_revents(events_[i].events);
        // 将channel存入活跃连接数组
        activeChannels->push_back(channel);
    }
}

// 更新channel通道
void EpollPoller::update(int operation, Channel *channel)
{
    // 这里主要就是根据operation: EPOLL_CTL_ADD MOD DEL来具体的调用epoll_ctl更改这个channel对应的fd在epoll上的监听事件
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events(); // events()函数返回fd感兴趣的事件。
    event.data.ptr = channel;         // 这个epoll_event.data.ptr是给用户使用的，附带数据
    int fd = channel->fd();
    if (epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        else
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
    }
}

TimeStamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_DEBUG("func=%s => fd total count: %lu \n", __FUNCTION__, channels.size());

    // 等待事件发生
    int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    TimeStamp now(TimeStamp::now());
    if (numEvents > 0)
    {
        LOG_DEBUG("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())        // 如果有活动的连接数量大于我们的events_能承载的数量，就要对events_扩容
            events_.resize(events_.size() * 2); // 反正我们用的是LT模式，那些没被添加进activeChannels的通道后面还会有机会被添加进来的。
    }
    else if (numEvents == 0)
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}
