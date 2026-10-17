
#include "Poller.h"
#include "EpollPoller.h"

// 相当于把 poller.h 当做自己的头文件
// 实现 poller.h 中声明过的 new default poller。
// 作用是替调用者选择并创建一种具体的 Poller,当前只有epoll可选

// 使用普通函数创建并返回一个新对象，称为工厂函数。

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    return new EPollPoller(loop);
}




































