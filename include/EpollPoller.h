
#pragma once
#include <sys/epoll.h>
#include <vector>
#include "Poller.h"

class EPollPoller : public Poller
{

public:

    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);
    static const int kInitEventListSize = 16;

    using EventList = std::vector<epoll_event>;
    // epoll_events用于配置或接收文件描述符（fd）上的事件信息
    // 这里的 epollevents 结构体第2个参数 data ,
    // 本项目使用它的 data.ptr 保存对应的 Channel*
    // 第一个参数应该和 channel 的私有变量 event 相等。表示关注的事情。

    int epollfd_;
    EventList events_;
};
















