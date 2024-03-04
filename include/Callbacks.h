/*
 * @Author: lvxr
 * @Date: 2024-03-04 17:26:48
 * @LastEditTime: 2024-03-04 17:26:49
 */
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <memory>
#include <functional>
class Buffer;
class TcpConnection;
class TimeStamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, TimeStamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

#endif
