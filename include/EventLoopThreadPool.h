
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

// 线程池持有N个EventLoopThread的unique指针，并在新连接到来时选出一个loop

class EventLoopThreadPool : noncopyable
{

public:

    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads);
    // 额外创建numThreads个工作线程（子线程）
    
    void start();                                                                                                                                                                     
    void start(const ThreadInitCallback &cb);     

    EventLoop* getNextLoop();
    // 根据next下标获取下一个loop
    std::vector<EventLoop*> getAllLoops();
    //  返回eventloop地址列表的副本

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
    // 调用方（主线程）的loop地址
    // 本类的构造到析构的所有方法都限制在同一线程中进行
    std::string name_;

    bool started_;
    int numThreads_;

    size_t next_;
    // 下一次选择的下标（由主线程修改）

    std::vector<std::unique_ptr<EventLoopThread>> threads_;
// 使用clear移除所有uniqueptr，自动触发EventLoopThread析构（调用stop()）
    std::vector<EventLoop*> loops_;
// 从各自 startLoop 返回值取得的对应eventloop
// 以上二者的下标一一对应

};







