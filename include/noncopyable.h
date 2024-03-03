/*
 * @Author: lvxr
 * @Date: 2024-03-02 15:40:38
 * @LastEditTime: 2024-03-03 14:05:04
 */
#ifndef NON_COPYABLE_H
#define NON_COPYABLE_H

namespace muduo
{
    // 该类派生类可以正常构造和析构，但是不能拷贝构造和赋值
    class noncopyable
    {
    public:
        // 禁止拷贝构造
        noncopyable(const noncopyable &) = delete;

        // 禁止等号赋值
        noncopyable &operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}

#endif