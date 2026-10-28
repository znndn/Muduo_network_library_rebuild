#include "EventLoopThread.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include <cassert>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const std::string& name)
    : loop_(nullptr), 
      exiting_(false),
      ownerThreadId_(CurrentThread::GetTID()),
      thread_([this]() { threadFunc(); }, name), 
      // 调用thread的构造函数，
      // 需要使用lambda而非直接传入threadFunc是因为后者隐式携带*this，而非<void()>
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

void EventLoopThread::stop()
{
    assert(CurrentThread::GetTID() == ownerThreadId_);
    // 确保在父线程中执行该任务
    if (exiting_==true || thread_.started()==false)
    {
        return;
    }
    else exiting_=true;

    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,[this] {return loop_!=nullptr;});

    loop_->queueInLoop([this](){loop_->quit();});
    thread_.join(); // 主线程等待子线程（loop线程）完成返回后再继续，此时子线程已经
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; 
    // 注意整个loop类都是一个局部变量，因此离开threadfunc自动析构
    if (callback_!=nullptr)
    {
        callback_(&loop);
    }

    // TODO B：在本类 mutex_ 下发布地址，再通知等待者。
    std::lock_guard<std::mutex> lock(mutex_);
    loop_=&loop;
    cond_.notify_one();

    loop.loop();

    // 结束loop循环，该quit了
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}




