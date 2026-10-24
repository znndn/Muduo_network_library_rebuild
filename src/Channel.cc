
#include <functional>
#include <memory>
#include <utility>
#include <sys/epoll.h>
#include "noncopyable.h"
#include "timestamp.h"
#include "EventLoop.h"
#include "Channel.h"

class EventLoop;

Channel::Channel(EventLoop* loop, int fd):fd_(fd),loop_(loop)
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
        // channel也可能服务于Acceptor和Eventloop（wakeup channel），因此没有绑定不能说明没用
        handleEventWithGuard(receiveTime);
    }
    else
    {
        std::shared_ptr<void> guard=tie_.lock();
        // shared_ptr<void> 的意思是这个指针不暴露对象的具体类型，但仍参与共有的寿命管理。
        // 对于不确定的对象类型，可以这么写。
        // weak_ptr::lock() 尝试取得 shared_ptr；
        // 对象已经过期时得到空指针。

        // 本来就是one loop one thread，因此不是为了加锁，是为了延长tcpconnnection的生命周期
        if (guard) 
        // 这一步 guard 自己也持有 shared_ptr，
        // 因此如果在回调的过程中，之前的 shared_ptr 全部被释放，该内容也不会被删除。
        {
            handleEventWithGuard(receiveTime);
        }
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    //EPOLLHUP 指的是hangup 读写都关闭,注意同时没有需要收的信息
    // 这时候就应该执行回调关闭的代码块
    if ((revents_&EPOLLHUP)!=0 && (revents_&EPOLLIN)==0)
    // 注意判断它是否相等的优先级高于按位和的优先级，因此要加上括号。
    {
        // 调用前检查对应回调是否已设置!
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    if ((revents_&EPOLLERR)!=0)
    {
        if (errorCallback_)
            errorCallback_();
    }
    if ((revents_&EPOLLIN)!=0 || (revents_&EPOLLPRI)!=0)
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
    if ((revents_&EPOLLOUT)!=0)
    {
        if (writeCallback_)
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
// 要记得读关注，同时需要设定 epoll pri。不只是 epoll in。
{
    events_ = events_ | EPOLLIN;
    events_ = events_ | EPOLLPRI;
    update();
}
void Channel::disableReading()
{
    events_ = events_ & ~EPOLLIN;
    events_ = events_ & ~EPOLLPRI;
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
// 移除所有关注，请注意这时候 index 不变。
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

EventLoop* Channel::ownerLoop()
{
    return loop_;
}

void Channel::setReadCallback(ReadEventCallback cb)
// 具体的回调逻辑
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

















