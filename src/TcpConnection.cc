
#include <iostream>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include "noncopyable.h"
#include "InetAddress.h"
#include "Socket.h"
#include "CallBacks.h"
#include "buffer.h"
#include "timestamp.h"
#include "TcpConnection.h"
#include "Channel.h"

// 在 acceptor 得到 connfd 之后创建。
TcpConnection::TcpConnection(
                EventLoop* loop,
                const std::string& nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr):
                
                loop_(loop),name_(nameArg),
                socket_(std::make_unique<Socket>(sockfd)),
                channel_(std::make_unique<Channel>(loop,sockfd)),
                localAddr_(localAddr),
                peerAddr_(peerAddr)
{
    setState(Connecting);
    reading_=true;
    channel_->setReadCallback([this](Timestamp receiveTime){handleRead(receiveTime);});
    channel_->setWriteCallback([this](){handleWrite();});
    channel_->setCloseCallback([this](){handleClose();});
    channel_->setErrorCallback([this](){handleError();});
    // 在此处设置对应 channel 的回调函数，让 channel 回调的时候可以找到自己的 TCP connection。
}   

TcpConnection::~TcpConnection()
{
    socket_.reset();
    channel_.reset();
    setState(Disconnecting);
}

// 构造后会被TcpServer调用,关联寿命，启用读、通知应用。
void TcpConnection::connectEstablished()
{
    
}





























