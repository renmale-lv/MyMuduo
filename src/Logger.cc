/*
 * @Author: lvxr
 * @Date: 2024-03-02 17:09:06
 * @LastEditTime: 2024-03-02 19:28:14
 */
#include "Logger.h"

#include <iostream>

#include "Timestamp.h"

Logger &Logger::Instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    level_ = level;
}

void Logger::log(std::string msg)
{
    // 写日志
    //[级别信息] time : msg
    switch (level_)
    {
    case INFO:
    {
        std::cout << "[INFO]";
        break;
    }
    case ERROR:
    {
        std::cout << "[ERROR]";
        break;
    }
    case FATAL:
    {
        std::cout << "[FATAL]";
        break;
    }
    case DEBUG:
    {
        std::cout << "[DEBUG]";
        break;
    }
    default:
        break;
    }
    // 打印时间和消息
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}
