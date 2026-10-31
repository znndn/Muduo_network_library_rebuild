
#pragma once

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int, const InetAddress&)>;
    // 表示链接fd和链接的ip以及端口的结构体

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { 
        newConnectionCallback_ = cb; 
    }

    bool listenning() const 
    {
        return listenning_; 
    }

    void listen();

private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_; //不要时作为成员函数自动析构
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};




















