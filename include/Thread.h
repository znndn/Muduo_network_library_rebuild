
#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <sys/types.h>
#include "noncopyable.h"

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const 
    { 
        return started_; 
    }
    pid_t tid() const 
    { 
        assert(started_); return tid_; 
    }
    const std::string& name() const 
    { 
        return name_; 
    }
    static int numCreated() 
    { 
        return numCreated_.load(); 
    }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
// 用来记录当前线程是否已经被调用过 join()等待回收/汇合
// join() 用于阻塞当前的线程（通常是主线程调用别的线程.join()），直到被调用的目标线程执行完毕
// （或达到指定超时时间），当前线程才会继续往下执行。因为一个线程多次被join是违规的。

    std::thread thread_;
    // 子线程
    pid_t tid_; // 赋值后应当与所属loop的tid一致 
    ThreadFunc func_;
    // 保存新线程创建后实际要执行的业务任务，构造时传入具体逻辑
    // 此处稍后用于构造EventLoop loop，由loop同一线程调用
    std::string name_;

    std::mutex mutex_;
    std::condition_variable cond_; 
    // 和互斥锁配合使用的条件变量，
    static std::atomic_int numCreated_;
    // 所有thread共用的创建thread数量，不初始化不用inline声明
};