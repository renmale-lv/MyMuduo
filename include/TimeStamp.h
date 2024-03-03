/*
 * @Author: lvxr
 * @Date: 2024-03-02 16:02:33
 * @LastEditTime: 2024-03-03 14:38:31
 */
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <string>
#include <time.h>

// 时间戳类，封装一些相关的函数
class TimeStamp
{
public:
    // 带参构造函数，禁止隐式转换
    explicit TimeStamp(int64_t secondSinceEpoch);

    // 静态函数，返回当前时间
    static TimeStamp now();

    // 将时间戳转换为string类型
    std::string toString() const;

private:
    // 时间戳，单位为秒
    int64_t secondSinceEpoch_;
};

#endif
