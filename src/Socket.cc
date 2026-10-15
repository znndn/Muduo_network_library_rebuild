
#include<iostream>
#include <cstring>
#include"Socket.h"
#include "InetAddress.h"
#include "Logger.h"

Socket::Socket(int sockfd): sockfd(sockfd)
// 允许使用参数构造来初始化修改const变量！但是此处不允许使用this指针
{
}

Socket::~Socket()
{
    ::close(sockfd);
    // RAII 的重点，一定要记得析构的时候关闭fd句柄，析构函数要保证收回所有资源
}

int Socket::Getfd() const
{
    return sockfd;
}

void Socket::BindAddress(const InetAddress &currAddr)
{
    int result=::bind(sockfd,reinterpret_cast<const struct sockaddr*>(currAddr.getSocketAddress()),sizeof(struct sockaddr_in));
    // 没有现代C++的时代，以前的结构体使用struct sockaddr而非struct sockaddr_in
    // 结构相同，直接重解释转换即可。
    if (result!=0) 
    {
        log_fatal(" bind address %d fail",sockfd);
    }
}

void Socket::listen()
{
    int result=::listen(sockfd,4096);
    if (result==-1)
    {
        log_fatal("failed to listen to sockfd %d",sockfd);
    }
}

int Socket::accept(InetAddress *clientAddr)
{
    sockaddr_in tempSockInfo;
    socklen_t InfoLength=sizeof(tempSockInfo);
    memset(&tempSockInfo,0,sizeof(tempSockInfo));
    int result=::accept4(sockfd,
        reinterpret_cast<struct sockaddr*>(&tempSockInfo),
        &InfoLength,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (result>=0)
    {
        clientAddr->setSocketAddress(tempSockInfo);
    }
    return result;
}

void Socket::shutDownWrite()
{
    if (::shutdown(sockfd,SHUT_WR)<0)
    {
        log_error("shut down socket fd %d fail",sockfd);
    }
}

void Socket::setReuseAddr(bool on)
{
    // 允许一个套接字强制绑定到一个已被其他套接字使用的端口。这对于需要重启并绑定到相同端口的服务器应用程序非常有用。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    // 允许同一主机上的多个套接字绑定到相同的端口号。这对于在多个线程或进程之间负载均衡传入连接非常有用。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    // 启用在已连接的套接字上定期传输消息。如果另一端没有响应，则认为连接已断开并关闭。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}




























