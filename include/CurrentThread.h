#pragma once
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int threadID; // 0表示没有获取tid
    void cacheThreadID();
    inline int GetTID()
    {
        if (__builtin_expect(threadID==0,0))
//         long __builtin_expect(long exp, long c);
// • exp：整型表达式，通常是 if 条件判断中的表达式。
// • c：编译时常量，表示你期望 exp 更有可能等于的值。
// • 含义：告诉编译器，exp == c 发生的概率非常大。
        {
            cacheThreadID();
        }
        return threadID;
    }
}