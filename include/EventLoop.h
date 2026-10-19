
#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include "noncopyable.h"
#include "timestamp.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:

    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();
    Timestamp pollReturnTime() const;
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    void wakeup();
    // 由别的线程调用，修改eventfd让正在epollwait的当前线程退出

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);
    bool isInLoopThread() const;

private:

    void handleRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 正在运行
    std::atomic_bool quit_; 
// 退出evntloop请求,收到退出请求以后，当前这一轮仍可能没有执行完

    const pid_t threadId_;
    Timestamp pollReturnTime_;
    // 最近一次poll返回记录的时间
    std::unique_ptr<Poller> poller_;
    // eventloop唯一持有的poller，派生epollpoller

    int wakeupFd_;
    // 用于别的线程唤醒当前正在epollwait阻塞的线程去做事
    // 可能是mainreactor分配新的channel，发送数据，退出
    std::unique_ptr<Channel> wakeupChannel_;
    // 包装wakeupfd的普通channel

    ChannelList activeChannels_;
    // 只保存本轮就绪对象；每轮重新生成
    std::atomic_bool callingPendingFunctors_;
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;

};