/*
 * @Author: lvxr
 * @Date: 2024-03-03 14:09:06
 * @LastEditTime: 2024-03-03 14:45:31
 */
#ifndef EPOLL_POLLER_H
#define EPOLL_POLLER_H

#include <vector>
#include <sys/epoll.h>

#include "TimeStamp.h"
#include "Poller.h"

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller();

    // 重写基类Poller的抽象方法
    // 进行IO复用，wait
    TimeStamp poll(int timeoutMs, ChannelList *activeChannels) override;
    // 更新一个channel
    void updateChannel(Channel *channel) override;
    // 将一个channel移除
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16; // 事件列表的初始长度

    // 填充活跃的连接，将活跃的连接存入activeChannels数组中
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    // 更新channel通道
    void update(int operation, Channel *channel);

    int epollfd_; // epoll事件循环本身还需要一个fd

    using EventList = std::vector<epoll_event>;
    EventList events_;
};

#endif