/*
 * @Author: lvxr
 * @Date: 2024-03-04 13:55:11
 * @LastEditTime: 2024-03-04 14:43:22
 */

#include "InetAddress.h"

#include <strings.h>
#include <string.h>

InetAddress::InetAddress(const sockaddr_in &addr) : addr_(addr) {}

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);                  
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // inet_addr()用来将参数cp 所指的网络地址字符串转换成网络所使用的二进制数字
}

string InetAddress::toIp() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); // 将数值格式转化为点分十进制的字符串ip地址格式
    return buf;
}
string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}