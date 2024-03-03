/*
 * @Author: lvxr
 * @Date: 2024-03-03 14:50:15
 * @LastEditTime: 2024-03-03 14:59:08
 */
#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
            // 通过linux系统调用，获取当前线程的tid值
            t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
    }
}