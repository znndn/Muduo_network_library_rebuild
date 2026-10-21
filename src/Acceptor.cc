
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
    acceptSocket_.BindAddress(listenAddr);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptChannel_.setReadCallback([this](Timestamp) { handleRead(); });
    // 绑定acceptchannel的读回调readCallback_()，让其等效于this->handleRead()
    // this需要捕获，因为handleRead 虽然属于 Acceptor 类，
    // 但调用它仍然需要一个具体的 Acceptor 对象。 类里可能创建多个 Acceptor，回调得知道自己要调用哪一个。
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
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;                       
    int connfd = acceptSocket_.accept(&peerAddr); // accept实际上把真实对端地址写进去了
    if (connfd<0)
    {
        log_fatal("fail to fetch client InetAddress with errno %d",errno);
    }
    else
    {
        newConnectionCallback_(connfd,peerAddr);
    }
}

























