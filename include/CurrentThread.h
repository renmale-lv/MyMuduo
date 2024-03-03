/*
 * @Author: lvxr
 * @Date: 2024-03-03 14:50:03
 * @LastEditTime: 2024-03-03 14:53:53
 */
#ifndef CURRENT_THREAD_H
#define CURRENT_THREAD_H

#include <unistd.h>
#include <sys/syscall.h>

namespace Currentthread
{

    // 通过__thread 修饰的变量，在线程中地址都不一样，__thread变量每一个线程有一份独立实体，各个线程的值互不干扰
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
            cacheTid();
        return t_cachedTid;
    }

}

#endif