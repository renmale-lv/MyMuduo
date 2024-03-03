/*
 * @Author: lvxr
 * @Date: 2024-03-03 13:52:12
 * @LastEditTime: 2024-03-03 14:01:06
 */
#include "Poller.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop) {}

bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}