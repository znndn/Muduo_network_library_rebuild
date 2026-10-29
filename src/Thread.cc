#include "Thread.h"
#include "CurrentThread.h"
#include <utility>
// 为eventloop增加在哪个工作线程里创建和运行的外层管理

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false), 
      joined_(false), 
      tid_(0),
      func_(std::move(func)), 
      name_(name)
{
    assert(func_); // 若func_为空，运行阶段终止程序
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        join();
    }
}

void Thread::start()
{
    // 父线程创建
    assert(!started_); // 如果标记开始后再调用start，终止程序
    thread_ = std::thread([this]() 
    {
        // std::thread 创建新线程
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tid_ = CurrentThread::GetTID(); // 加锁防止写的时候读
        }
        cond_.notify_one(); // 用于随机唤醒一个正在等待该条件变量的线程
        func_(); // 调用func（实际上是threadFunc()）构造eventloop开始运行，结束后自动析构eventloop
    });
    // 父线程继续执行，与上述std::thread并行处理，不保证先后顺序。
    started_ = true;

    std::unique_lock<std::mutex> lock(mutex_); 
    // unique lock 和 lock guard 不同的地方在于前者可以临时释放然后再锁上，lock guard 并不行。
    cond_.wait(lock, [this]() { return tid_ != 0; });
    // cond_.wait 在每次唤醒后检查tid_!=0是否成立，仅仅在 tid_ != 0 成立退出wait
    // 进入等待之后会释放 mutex，只有在检查的时候会拿回来。
}

void Thread::join()
{
    assert(started_);
    assert(CurrentThread::GetTID() != tid_);// 确认当前运行在 loop 线程的父线程上（TID 与 loop 的 TID 不同）
    if (joined_)
    {
        return;
    }
    else
    {
        thread_.join();
        joined_ = true; 
    }
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        name_ = "Thread" + std::to_string(num); // Threadxxxx
    }
}