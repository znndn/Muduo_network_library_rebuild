#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include "noncopyable.h"
#include "Thread.h"

// A：调用 EventLoopThread::startLoop()
//    └─ 调用 thread_.start()
//       └─ 创建工作线程 B
//          └─ Thread 中保存的函数被执行
//             └─ 调用 EventLoopThread::threadFunc()

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThread(
        const ThreadInitCallback& cb = ThreadInitCallback(),
        const std::string& name = std::string());

    ~EventLoopThread();

    EventLoop* startLoop();
    // 父线程启动子线程（loop所在线程）
    void stop();
    // 父线程提交停止子线程的请求（作为任务队列中的一员）

private:
    void threadFunc();
    // 贯穿子线程（loop线程）全部的生命周期的函数（直到loop线程要移除了才会返回）
    // 在工作线程 B 中创建并运行一个专属的 EventLoop，开启loop循环

    EventLoop* loop_;
    // 对应同thread loop的指针

    bool exiting_;
    // 该线程是否开始停止
    const pid_t ownerThreadId_;
    // 父线程tid
    Thread thread_;
    // 保存本线程所对应的类,EventLoopThread构造和赋值thread类
    // （在EventLoopThread自己构造的时候赋值func实际执行threadFunc()的逻辑）
    // 并通过thread类的方法开始具体子线程eventloop的建立和运行

    std::mutex mutex_;
    std::condition_variable cond_;
    // 保护 loop 地址

    ThreadInitCallback callback_;
    // loop初始化回调，loop所属线程构造loop后调用
    // 读取线程信息、完成 loop 的初始化
};