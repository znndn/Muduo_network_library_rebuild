
#include <iostream>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include "noncopyable.h"
#include "Logger.h"
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
                peerAddr_(peerAddr),
                highWaterMark_(64 * 1024 * 1024)
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
    setState(Disconnected);
}

// 构造后会被TcpServer调用,关联寿命，启用读、通知应用。
void TcpConnection::connectEstablished()
{
    setState(Connected);
    channel_->tie(shared_from_this()); 
    channel_->enableReading();
    log_info("creating tcp connection on fd=%d state=%d\n", channel_->fd(), static_cast<int>(state_));
    if(connectionCallback_==nullptr)
    {
        return;
    }
    connectionCallback_(shared_from_this());
    // 通知应用层连接状态发生改变，为空则没有通知处理
}
// 彻底将 Channel 从 Poller/epoll 树中注销，并作为生命周期最后一次资源清理
void TcpConnection::channelDistroy()
{
    if (state_!=Disconnected) 
    //为什么会有这种情况？一般先让channel回调handleclose，然后上层再调conndistroy啊，此时必然Disc
    {    
        setState(Disconnected);
        channel_->disableAll();
        closeCallback_(shared_from_this());
    }
    if (loop_->hasChannel(channel_.get())) 
    {
        loop_->removeChannel(channel_.get());
    }
} 

// 停止接收事件、通知用户连接断开，并通知 TcpServer 开始注销此连接
void TcpConnection::handleClose()
{
    if (state_==Disconnected) return;
    setState(Disconnected);
    log_info("start to close tcp connection on fd=%d state=%d\n", channel_->fd(), static_cast<int>(state_));
    channel_->disableAll();
    reading_=false;
    TcpConnectionPtr guard(shared_from_this());
    connectionCallback_(shared_from_this());
    closeCallback_(shared_from_this());
}
// 处理channel的错误回调，对非零 socketError 记录后进入 handleClose
void TcpConnection::handleError()
{
    int socketError = 0;
    socklen_t length = sizeof(socketError);
    if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &socketError, &length) < 0) 
    // 将 optname 参数设置为 SO_ERROR 来专门查询该 Socket 内部操作产生的错误码，会写入传入缓冲区socketError
    {
        socketError = errno;
    }
    log_error("Error %d occured on socket fd %d,closing fd",socketError,socket_->Getfd());
    handleClose();
}


























