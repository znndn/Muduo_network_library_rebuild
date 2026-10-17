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
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);
    bool isInLoopThread() const;

private:
    void handleRead();
    void doPendingFunctors();
    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;
    std::atomic_bool quit_;
    const pid_t threadId_;
    Timestamp pollRetureTime_;
    std::unique_ptr<Poller> poller_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;
    std::atomic_bool callingPendingFunctors_;
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;
};