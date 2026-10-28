#include "EventLoopThread.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include <cassert>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const std::string& name)
    : loop_(nullptr), 
      exiting_(false),
      ownerThreadId_(CurrentThread::GetTID()),
      thread_([this]() 
      { 
        threadFunc(); 
      }, name),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    stop();
}

EventLoop* EventLoopThread::startLoop()
{
    assert(CurrentThread::GetTID() == ownerThreadId_);
    // 保证在父线程上运行
    assert(!exiting_ && !thread_.started());
    // 保证不打算退出，还没运行start()
    thread_.start();

    // TODO A：在本类 mutex_ 下等待 loop_ 非空，返回这份地址。
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,[this] {return loop_!=nullptr;});
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    if (callback_!=nullptr)
    {
        callback_(&loop);
    }

    // TODO B：在本类 mutex_ 下发布地址，再通知等待者。
    std::lock_guard<std::mutex> lock(mutex_);
    loop_=&loop;
    cond_.notify_one();

    loop.loop();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}




