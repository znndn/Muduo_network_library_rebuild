
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include <CurrentThread.h>

namespace CurrentThread
{
    __thread int threadID; 
    // 这里thread的作用是让变量对于每个线程独立，而非作为全局变量等
    void CurrentThread::cacheThreadID()
    {
        if (CurrentThread::threadID==0)
        {
            int curr_thread_id=syscall(SYS_gettid);
            threadID = static_cast<pid_t>(curr_thread_id);
            // pid_t 是专门为了pid设计的int别名，用于识别和后续迁移
        }
    }
}