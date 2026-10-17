
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <errno.h>

#include "Poller.h"
#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

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
    int result=epoll_ctl(epollfd_, operation, TempEvent.events, &TempEvent);
    if (result==-1)
    {
        log_error("fail to update epoll fd %d with errno %d",epollfd_,errno);
    }
}

void EPollPoller::updateChannel(Channel* channel) 
// 根据channel index当前状况更新index并让内核开始监控（通过包装了epollctl的update）
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