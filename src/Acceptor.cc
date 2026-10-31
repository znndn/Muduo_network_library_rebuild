
#include <functional>
#include <sys/socket.h>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Logger.h"

// 有利于在参数列表中就获取fd
static int createNonblocking()
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                      IPPROTO_TCP);
    if (fd < 0) {
        log_fatal("create listen socket failed, errno=%d", errno);
    }
    return fd;
}
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport):
// listenAddr是监听（自己的）地址，不是客户端地址
    loop_(loop),
    acceptSocket_(createNonblocking()),
    acceptChannel_(loop,acceptSocket_.Getfd()),
    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.BindAddress(listenAddr);
    acceptChannel_.setReadCallback([this](Timestamp) { handleRead(); });
    // 绑定acceptchannel的读回调readCallback_()，让其等效于this->handleRead()
    // this需要捕获，因为handleRead() 是普通成员函数，不能脱离对象独立运行（即使只有一个acceptor）
    // 因为 this 指针总是函数隐式传递的第一个参数，不会写出来，但是缺少他会报错。

    // 注意类似这样的裸指针this反而不会增加引用计数（不会循环引用）
}

Acceptor::~Acceptor()
{
    if (loop_-> hasChannel(&acceptChannel_)==true)
    {
        acceptChannel_.disableAll();
        acceptChannel_.remove();    
    }
    // Acceptor 析构函数体结束，所有成员按声明的逆序自动析构：先析构 acceptChannel_，再析构 acceptSocket_
    // 因此无须手动析构函数
}

void Acceptor::listen()
{
    acceptSocket_.listen();
    listenning_=true;
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;                       
    int connfd = acceptSocket_.accept(&peerAddr); // accept实际上把真实对端地址写进去了
    if (connfd<0)
    {
        // 注意不是所有accept失败都是fatal，
        // 调用读回调是照例的，可能是因为socket fd等有更新，而非新连接让其返回
        // EAGAIN/EWOULDBLOCK 表示这次没有连接可接受
        auto saveError=errno;
        if (saveError == EAGAIN || saveError == EWOULDBLOCK|| 
            saveError == EINTR || saveError == ECONNABORTED)
            return;
        else log_fatal("fail to fetch client InetAddress with errno %d",saveError);
    }
    else
    {
        if (newConnectionCallback_==nullptr)
        {
            ::close(connfd);
            log_fatal("newConnectionCallback_ is nullptr on accept socket %d",connfd);
            return;
        }
        newConnectionCallback_(connfd,peerAddr);
    }
}

























