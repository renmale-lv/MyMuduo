/*
 * @Author: lvxr
 * @Date: 2024-03-04 13:55:04
 * @LastEditTime: 2024-03-05 15:02:13
 */
#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// ipv4地址类
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr);
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    const sockaddr_in *getSockAddr() const { return &addr_; }
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};

#endif