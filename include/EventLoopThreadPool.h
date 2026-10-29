
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{

public:

    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads);
    // 额外创建numThreads个工作线程（子线程）
    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    EventLoop* getNextLoop();
    std::vector<EventLoop*> getAllLoops();

    bool started() const 
    { 
        return started_; 
    }
    const std::string& name() const 
    { 
        return name_; 
    }

private:

    EventLoop* baseLoop_;
    // 主线程的loop
    std::string name_;
    bool started_;
    int numThreads_;
    size_t next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;

};







