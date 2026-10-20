
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
    auto result=epoll_create1(EPOLL_CLOEXEC);
    if (result==-1)
    {
        log_fatal("unable to create epoll fd");
    }
    else epollfd_=result;
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
    int result=epoll_ctl(epollfd_, operation, channel->fd(), &TempEvent);
    // 根据 operation添加或删除内核对它的监听，
    // 或者通过channel->events();修改内核对他关注的事项。

    // 一个 epoll 实例，也就是一个 epollwait 会监督多个 socket fd，
    // 相当于每个连接都是一个文件描述符socket fd，
    // 因此 epoll fd 跟 sockets fd 是不一样的。第3个参数应该使用 sockets fd。

    if (result==-1)
    {
        if (operation==EPOLL_CTL_DEL)
            log_error("fail to update epoll fd %d with errno %d",epollfd_,errno);
        else
            log_fatal("fail to update epoll fd %d with errno %d",epollfd_,errno);
    }
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) 
{
    int result=epoll_wait(epollfd_, &events_[0], events_.size(), timeoutMs);
    // 这里如果没事做就一直等，不需要立即返回，并持续监控epollctl注册的fd，这和非阻塞没关系
    auto CheckErrno=errno;
    auto curr=Timestamp::now();

    if (result==-1)
    {
        if (CheckErrno==EINTR) return curr; 
        //不记录EINTR信号中断的情况,例如Ctrl+C，进程终止信号等，不是wakeup！
        // EINTR 的全称是 Interrupted System Call（系统调用被中断）
        std::string now=curr.toString();
        log_error("epoll wait failed with errno %d at %s",CheckErrno, now.c_str());
        // %s 只能接收char* 需要c风格字符串
    }
    else if (result>0)
    {
        fillActiveChannels(result,activeChannels);
        // 每次 active channels 的数量是不确定的，也就是可能含有不定数量的文件描述符，
        // 不一定能填满 event list 的所有位置，也有可能比 event list 要更多，
        // 因此不能直接传入 event list 的大小，读到空位置会导致段错误。
        // 注意epollwait成功时会返回新的fd数量

        // 虽然Eventlist是可变stl，但是epollwait无法自动扩容，需要手动调整大小
        if (static_cast<long unsigned int>(result) == events_.size()) 
        {
            events_.resize(events_.size() * 2);
            // 不扩容也可以，会慢一点，内核允许一次只返回部分就绪 fd，一次空间不够会多次传输并从头开始覆盖原有的
        }
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
        if (channel->isNoneEvent())
        // 一定要在mod之前先检查是否移除关注了（此时index不变）
        // 如果在 channel.events=0 的情况下 mod 了，会导致非法参数的错误。
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(Deleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
            // 让内核通过struct epoll_event的events，修改关注的内容（由update更新）
        }
    }
    else // channel->index()==Deleted
    {
        channel->set_index(Added);
        update(EPOLL_CTL_ADD,channel);
        // 从移除恢复读关注
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





