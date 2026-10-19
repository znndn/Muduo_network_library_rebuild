
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <thread>
#include <sys/eventfd.h>
#include "noncopyable.h"
#include "timestamp.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

thread_local EventLoop* t_loopInThisThread = nullptr;
// thread_local（线程局部存储）：
// 通常的全局变量或静态变量是在所有线程间共享的，如果多个线程同时修改它，就会发生数据竞争
// 而加上 thread_local 后，每个线程都会复制一份该变量的独立副本。线程 A 修改了它，线程 B 的值完全不受影响。

// 构造函数体先检查当前线程这份记录：非空则报告致命错误，否则保存 this
// 用于判断是否有创建过，防止同一线程同时构造两个 EventLoop

class Channel;
class Poller;

int createEventfd()
{
    int result=::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    auto SaveError=errno;
    if (result==-1)
    {
        log_error("fail to create eventfd with errno %d",SaveError);
    }
    else
    {
        return result;
    }
}

EventLoop::EventLoop():
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupFd_)),
    threadId_(syscall(SYS_gettid))
{
    // 如果使用 make_unique 赋值poller_
    // 会尝试调用 Poller 的构造函数构造一个新的poller，
    // 而非将newDefaultPoller()的返回值转化为独占指针

    // 注意eventloop初始化显然是跑在子线程上的（subreactor），
    // 由主线程（mainreactor）先创建子线程，子线程再去创建eventloop，因此threadId_直接获取当前线程号即可
    looping_=false;
    quit_=false;

    if (t_loopInThisThread!=nullptr) log_fatal("found duplicate construction of EventLoop");
    else t_loopInThisThread=this;

    wakeupChannel_->setReadCallback([this](Timestamp) { handleRead(); });
    // 绑定对应channel的读回调，lambda捕捉自己loop的指针
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    wakeupChannel_.reset();
    // 让uniquptr释放wakeupchannel，并关闭eventfd
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

void EventLoop::loop()
{
    looping_=true;
    quit_=false;
    while (quit_==false)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(10000,&activeChannels_);
        // 接下来处理epollwait返回的活跃fd（每个使用channel的方法），poll可以保证activeChannels_没有多余元素
        for (auto elem: activeChannels_)
        {
            elem->handleEvent(pollReturnTime_);
        }
    }
    looping_=false;
}

void EventLoop::quit()
{
    quit_=true;
    if (looping_) wakeup();
}

bool EventLoop::isInLoopThread() const
{
    if (syscall(SYS_gettid)==threadId_) 
    // 注意区分Linux pid（上述）和C++进程id std::thread::getid
    return true;
    else return false;
}

Timestamp EventLoop::pollReturnTime() const
{
    return pollReturnTime_;
}

void EventLoop::wakeup()
// 注意要注册给内核的eventfd才能在epollwait得到监控，通过wakeupfd==eventfd在内核修改它
{
    uint64_t one=1;
    ssize_t n=::write(wakeupFd_,&one,sizeof(one));
    auto saveErrno=errno;
    if (n!=8) log_error("cannot wake up thread %d via wakeup fd %d correctly with errno %d"
        ,threadId_,wakeupFd_,saveErrno);
}

void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead()
{
    uint64_t count = 0;
    ssize_t n = ::read(wakeupFd_, &count, sizeof(count));
    // 注意系统调用的read会读出累计值，同时将计数归零，不必再write 0
    auto saveErrno=errno;
    if (n!=8) log_error("cannot reset wakeup fd %d correctly with errno %d"
        ,wakeupFd_,saveErrno);
}































