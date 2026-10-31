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

    for (int i = 0; i < numThreads_; i++)
    {
        // 通过构造函数保存cb，等到 EventLoopThread::threadFunc调用并执行cb
        auto thread = std::make_unique<EventLoopThread>(cb, name_ + std::to_string(i));

        EventLoop* loop = thread->startLoop();
        
        threads_.push_back(std::move(thread));
        loops_.push_back(loop);
    }

    // 如果用户没有设置多线程 (numThreads_ == 0) 且cb非空，说明是单线程的情况        
    if (numThreads_ == 0 && cb!=nullptr)
    
    {
        cb(baseLoop_);
        // 直接运行cb，因为baseloop早已启动
    }
}


EventLoop* EventLoopThreadPool::getNextLoop()
{
    assert(baseLoop_->isInLoopThread());
    assert(started_);

    if (loops_.size()==0) return baseLoop_;
    // 只是在没有分配子线程（loops_ 为空）的特殊情况下，为了保证接口可用而临时把 baseLoop_ 拿出来兜底
    else if (loops_.size()==1) return loops_[0];
    else
    {
        int toReturn;
        if (next_==loops_.size()-1)
        {
            toReturn=next_;
            next_=0;
        }
        else 
        {
            toReturn=next_;
            next_++;
        }
        return loops_[toReturn];
    } 
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    assert(baseLoop_->isInLoopThread());
    assert(started_);

    if (loops_.empty())
    {
        return std::vector<EventLoop*>{baseLoop_}; 
        
        // 空的的话返回baseloop的ventor，让接口可用
    }
    return loops_;
}













