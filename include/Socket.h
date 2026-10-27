
#pragma once
#include<iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include "noncopyable.h"
#include "InetAddress.h"

class InetAddress;

class Socket: noncopyable
{
    public:
    explicit Socket(int sockfd);
    ~Socket();

    int Getfd() const;
    void BindAddress(const InetAddress &currAddr);
    // 绑定当前sockfd和地址端口
    void listen();
    
    int accept(InetAddress *clientAddr);
    // 绑定对方sockfd和地址端口

    void shutDownWrite(); 
    // 半关闭,禁止写，但是不移除写缓冲区的内容。

    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

    private:
    const int sockfd;
};



























