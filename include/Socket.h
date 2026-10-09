
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

    int acceptGetConnfd(InetAddress *oppoaddr);
    // 建立新的连接时获得对方的InetAddress类型的数据，返回对应的connfd

    void shutDownWrite(); 
    // 半关闭

    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

    private:
    const int sockfd;
};



























