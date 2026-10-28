
#pragma once
#include <vector>
#include <unordered_map>
#include "timestamp.h"

class Channel;
class EventLoop;

class Poller
{
public:
    using ChannelList = std::vector<Channel*>;
    
    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    bool hasChannel(Channel* channel) const;
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    // map存放socket fd和channel指针,用于记录socket fd和自身channel的对应关系
    // map上即为这个线程全部的注册socket fd
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;

};










