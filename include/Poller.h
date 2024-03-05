/*
 * @Author: lvxr
 * @Date: 2024-03-03 13:52:02
 * @LastEditTime: 2024-03-05 15:03:31
 */

#ifndef POLLER_H
#define POLLER_H

#include <unordered_map>
#include <vector>

#include "noncopyable.h"
#include "Channel.h"

class Channel;
class EventLoop;

// IO复用纯虚类
class Poller : public muduo::noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller(){}

    // 进行IO复用
    virtual TimeStamp poll(int timeoutMs, ChannelList *ativateChannels) = 0;
    // 更新通道
    virtual void updateChannel(Channel *channel) = 0;
    // 移除通道
    virtual void removeChannel(Channel *channel) = 0;

    // 判断一个poller里面有没有这个channel
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;

    ChannelMap channels_;

private:
    EventLoop *ownerLoop_;
};

#endif