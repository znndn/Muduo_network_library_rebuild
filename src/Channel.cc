
#include <functional>
#include <memory>
#include <utility>
#include <sys/epoll.h>
#include "noncopyable.h"
#include "timestamp.h"
#include "EventLoop.h"
#include "Channel.h"

class EventLoop;

Channel::Channel(EventLoop* loop, int fd):loop_(loop),fd_(fd)
{
    events_=0;
    revents_=0;
    index_=-1;
    tied=false;
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied==false) //表示没有调用过函数tie（），也就是不可能绑定了上层
    {
        handleEventWithGuard(receiveTime);
    }
    else
    {
        std::shared_ptr<void> guard=tie_.lock();
        if (guard) 
        {
            handleEventWithGuard(receiveTime);
        }
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    //EPOLLHUP 指的是hangup 读写都关闭,注意同时没有需要收的信息
    // 这时候就应该执行回调关闭的代码块
    if (revents_&EPOLLHUP!=0 && revents_&EPOLLIN==0)
    {
        closeCallback_();
    }
    else if (revents_&EPOLLERR!=0)
    {
        errorCallback_();
    }
    else if (revents_&EPOLLIN!=0 || revents_&EPOLLPRI!=0)
    {
        readCallback_(receiveTime);
    }
    else if (revents_&EPOLLOUT!=0)
    {
        writeCallback_();
    }
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied = true;
}
int Channel::fd() const
{
    return fd_;
}
int Channel::events() const
{
    return events_;
}
void Channel::set_revents(int revt)
{
    revents_=revt;
}
int Channel::index()
{
    return index_;
}
void Channel::set_index(int idx)
{
    index_=idx;
}


void Channel::enableReading()
{
    events_ = events_ | EPOLLIN;
    update();
}
void Channel::disableReading()
{
    events_ = events_ & ~EPOLLIN;
    update();
}
void Channel::enableWriting()
{
    events_ = events_ | EPOLLOUT;
    update();
}
void Channel::disableWriting()
{
    events_ = events_ & ~EPOLLOUT;
    update();
}
void Channel::disableAll()
{
    events_=0;
    update();
}

bool Channel::isNoneEvent() const
{
    return events_==0?1:0;
}
bool Channel::isWriting() const
{
    return (events_&EPOLLOUT)!=0?true:false;
}
bool Channel::isReading() const
{
    return (events_&EPOLLIN)!=0?true:false;
}

void Channel::setReadCallback(ReadEventCallback cb)
{
    readCallback_=std::move(cb);
    // 因为read需要单独接收时间戳，所以cb类型不同
}
void Channel::setWriteCallback(EventCallback cb)
{
    writeCallback_=std::move(cb);
}
void Channel::setCloseCallback(EventCallback cb)
{
    closeCallback_=std::move(cb);
}
void Channel::setErrorCallback(EventCallback cb)
{
    errorCallback_=std::move(cb);
}


void Channel::update()
{
    loop_->updateChannel(this); 
}
void Channel::remove()
{
    loop_->removeChannel(this);
}

















