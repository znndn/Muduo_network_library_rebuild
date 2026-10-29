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
    // 进入 wait 之后会立刻检查一次 lock 的值是否符合要求，如果符合就直接持锁进入下一步。
    // 这里虽然 wait 返回之后仍然持有锁，但后面loop return会因为整个函数的结束而自动析构解锁
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

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock,[this] {return loop_!=nullptr;});
    // 注意这里的 wait，只有在进入等待之后才会释放锁，
    // 其余情况包括检查loop的值,和检查完毕符合条件并返回，都会持有锁。
    // 也就是说，这里如果不用大括号自动析构锁的话，锁是不会在检查正确返回后自动释放的。
    }

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
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_=&loop;
    }
    cond_.notify_one();
    loop.loop();

    // 结束loop循环，该quit了
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}




