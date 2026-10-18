
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <string>

#include "Poller.h"
#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include "timestamp.h"

constexpr int New=-1;
constexpr int Added=1;
constexpr int Deleted=2;

EPollPoller::EPollPoller(EventLoop* loop):Poller(loop)
{
    events_.resize(kInitEventListSize);
    if (epoll_create1(EPOLL_CLOEXEC)==-1)
    {
        log_error("unable to create epoll fd");
    }
}
EPollPoller::~EPollPoller() 
{
    ::close(epollfd_);
}

void EPollPoller::update(int operation, Channel* channel)
{
    epoll_event TempEvent;
    TempEvent.events=channel->events();
    TempEvent.data.ptr=channel;
    // 让数据部分存储channel指针
    // 此时Channel* 隐式转换为了泛型指针 void* 交给内核保管
    int result=epoll_ctl(epollfd_, operation, TempEvent.events, &TempEvent);
    if (result==-1)
    {
        log_error("fail to update epoll fd %d with errno %d",epollfd_,errno);
    }
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) 
{
    int result=epoll_wait(epollfd_, &events_[0], events_.size(), timeoutMs);
    auto curr=Timestamp::now();

    if (result==-1)
    {
        if (errno==EINTR) return curr; //不记录打断的情况
        std::string now=curr.toString();
        log_error("epoll wait failed with errno %d at %s",errno,now);
    }
    else if (result>0)
    {
        fillActiveChannels(events_.size(),activeChannels);
        // 要扩大哪个数组？我不明白。
        // 看起来 ChannelList* activeChannels 跟 epoll wait 处理的 events_ 都是可变的大小
    }
    return curr;
}

void EPollPoller::updateChannel(Channel* channel) 
// 根据channel index当前状况更新index并让内核处理对应的epollfd_（通过包装了epollctl的update）
{
    if (channel->index()==New)
    {
        channels_[channel->fd()]=channel;
        channel->set_index(Added);
        update(EPOLL_CTL_ADD,channel);
    }
    else if (channel->index()==Added)
    {
        update(EPOLL_CTL_MOD,channel);
        // 让内核通过struct epoll_event的events，修改关注的内容（由update更新）
    }
    else if (channel->index()==Deleted)
    {
        channel->set_index(Added);
        update(EPOLL_CTL_ADD,channel);
        // 从移除恢复读关注
    }
    else if (channel->isNoneEvent())
    {
        update(EPOLL_CTL_DEL, channel);
        channel->set_index(Deleted);
    }
    else
    {
        update(EPOLL_CTL_MOD, channel);
    }
}
void EPollPoller::removeChannel(Channel* channel)
{
    channels_.erase(channel->fd());
    if (channel->index()==Added)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(New);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    // activeChannels_的EventLoop在等待前已经清空，需要收集本轮就绪Channel(来自events_)的地址
    for (int i=0;i<numEvents;i++)
    {
        static_cast<Channel*>(events_[i].data.ptr)->set_revents(events_[i].events);
        activeChannels->push_back(static_cast<Channel*>(events_[i].data.ptr));
    }   
}