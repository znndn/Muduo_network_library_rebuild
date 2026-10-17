
#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <sys/epoll.h>
#include "noncopyable.h"
#include "timestamp.h"
#include "EventLoop.h"

class EventLoop;

class Channel: noncopyable
{
    public:

    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
//相当于只接受timestamp的可调用对象
//如果没有被赋值，转换为 bool 时为 false；                                                                                         
//一旦调用了 setReadCallback(...) 绑定了可调用对象，转换为 bool 就为 true。   
    Channel(EventLoop* loop, int fd);
    ~Channel()=default;

    void handleEvent(Timestamp receiveTime);
    // 根据本次结果，调用此前保存的函数

    void setReadCallback(ReadEventCallback cb);
    // 写出可读时候的逻辑,
    // 此处具体的实现逻辑是由传入参数 cb 提供的，不会在 channel.cc 和 h 文件中定义。
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    void tie(const std::shared_ptr<void>& obj);
    int fd() const;
    int events() const;
    void set_revents(int revt);
    int index();
    void set_index(int idx);

    void enableReading();
    // 提出关注是否可读
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();
    
    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    EventLoop* ownerLoop();// get ownerloop

    void remove();

    private:

    void update(); // 用于更新内核
    void handleEventWithGuard(Timestamp receiveTime);

    static const int NoneEvent=0;
    static const int ReadEvent=EPOLLIN|EPOLLPRI;
    // 设置读活动关注的内容，使用按位或表示 同时关注 revents中表示同时发生
    static const int WriteEvent=EPOLLOUT;

    const int fd_;
    int events_; // 当前关注的事件
    int revents_; // 实际发生的事件
    int index_; 
    // 被 epollpoller 调用.
    // 记录这个 Channel 目前登记到了新 channel，已登记和已删除中的哪一步”

    std::weak_ptr<void> tie_;
    bool tied;

    EventLoop *loop_; // 保存所属的eventloop的地址，方便channel请求

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};