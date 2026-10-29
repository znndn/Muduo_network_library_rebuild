#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include <cassert>
#include <utility>

EventLoopThreadPool::EventLoopThreadPool
    (EventLoop* baseLoop,const std::string& nameArg)
    :   baseLoop_(baseLoop), 
        name_(nameArg), 
        started_(false),
        numThreads_(0), 
        next_(0)
{
    assert(baseLoop_ != nullptr);
    assert(baseLoop_->isInLoopThread());
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    assert(baseLoop_->isInLoopThread());
    loops_.clear();   // 移除存储的EventLoop*（在EventLoopThread::threadFunc中栈自动回收）
    threads_.clear(); // 析构EventLoopThreads
}

void EventLoopThreadPool::setThreadNum(int numThreads)
{
    assert(baseLoop_->isInLoopThread());
    assert(!started_ && numThreads >= 0);
    numThreads_ = numThreads;
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(baseLoop_->isInLoopThread());
    assert(!started_);

    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        auto thread = std::make_unique<EventLoopThread>(cb, name_ + std::to_string(i));
        
        EventLoop* loop = thread->startLoop();
        threads_.push_back(std::move(thread));
        loops_.push_back(loop);
    }

    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}


EventLoop* EventLoopThreadPool::getNextLoop()
{
    assert(baseLoop_->isInLoopThread());
    assert(started_);

    if (loops_.size()==0) return baseLoop_;
    else
    {
        int toReturn=next_;
        if (next_!=loops_.size())
        {
            next_++;
        }
        else next_=0;
        return loops_[toReturn];
    } 
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    return loops_;
}














