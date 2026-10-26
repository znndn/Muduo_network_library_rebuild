
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
    reading_=false;
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
    reading_=true;
    log_info("creating tcp connection on fd=%d state=%d\n", channel_->fd(), static_cast<int>(state_));
    if(connectionCallback_!=nullptr)    // 为空则没有通知处理，避免异常提前检查
    {
        connectionCallback_(shared_from_this());
    }

}
// 彻底将 Channel 从 Poller/epoll 树中注销，并作为生命周期最后一次资源清理
void TcpConnection::channelDistroy()
{
    if (state_!=Disconnected) 
    // 出错或者EOF时让channel回调handleclose，然后上层再调conndistroy清理
    // 如果主动结束，不会调用handleclose，而是直接channelDistroy，因此此时需要完成handleClose的职能
    {    
        setState(Disconnected);
        log_info("close tcp connection on fd=%d state=%d\n", channel_->fd(), static_cast<int>(state_));
        channel_->disableAll();
        reading_=false;
        if(connectionCallback_!=nullptr) 
        {
            connectionCallback_(shared_from_this());
        }
        // closeCallback正是让持有者安排清理的（调用channelDistroy）
    }
    if (loop_->hasChannel(channel_.get())) 
    {
        loop_->removeChannel(channel_.get());
    }
} 

// handleRead 进入时，用户态缓冲区 Buffer 未必为空，之前的包可能没有消费完
void TcpConnection::handleRead(Timestamp receiveTime)
{
    if (state_==Disconnected || channel_->isReading()==false) return;
    int Errors;
    int result = inputBuffer_.readFd(socket_->Getfd(),&Errors);
    if (result==-1)
    {
        if (Errors==EAGAIN || Errors==EWOULDBLOCK) return;
        else if (Errors==EINTR) 
        {
            handleRead(receiveTime);
            return;
        }
        handleError();
        return;
    }
    else if (result==0) // 识别 EOF 后逻辑关闭
    {
        handleClose();
    }
    else
    {
        if (messageCallback_!=nullptr)
        {
            messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
        }
        else
        {
            inputBuffer_.useAllData();
        }
    }
}
void TcpConnection::handleWrite()
{
// epollwait 返回时，channel针对写关注的fd调用handlewrite对应的回调函数，outputbuffer也需要消耗缓冲区
    if (state_==Disconnected || channel_->isWriting()==false) return;
    int Error;
    int result = outputBuffer_.writeFd(socket_->Getfd(),&Error);
    if (result==-1)
    {
        if (Error== EAGAIN|EWOULDBLOCK)
        {
            channel_->enableWriting();
            return;
        }
        else if (Error==EINTR)
        {
            handleWrite();
            return;
        }
        else
        {
            handleClose();
        }
    }
    else if (result==0)
    {
        channel_->enableWriting();
        return;
    }
    else
    {
        if (result<outputBuffer_.readableBytes())
        {
            outputBuffer_.useData(result);
            channel_->enableWriting();
            return;
        }
        else if (result==outputBuffer_.readableBytes())
        {
            outputBuffer_.useAllData();
            channel_->disableWriting();
            return;
        }
    }
}
// 停止接收事件、通知用户连接断开，并通知 TcpServer 开始注销此连接
void TcpConnection::handleClose()
{
    if (state_==Disconnected) return;
    setState(Disconnected);
    log_info("close tcp connection on fd=%d state=%d due to failure\n", channel_->fd(), static_cast<int>(state_));
    channel_->disableAll();
    reading_=false;
    TcpConnectionPtr guard(shared_from_this());
    // 记得每次连接回调前都要判空，防止这里抛异常后面的不做了
    if(connectionCallback_!=nullptr) 
    {
        connectionCallback_(shared_from_this());
    }
    closeCallback_(shared_from_this());
}
// 处理channel的错误回调，对非零 socketError 记录后进入 handleClose
void TcpConnection::handleError()
{
    if (state_==Disconnected) return;
    int socketError = 0;
    socklen_t length = sizeof(socketError);
    if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &socketError, &length)==-1) 
    // 将 optname 参数设置为 SO_ERROR 来专门查询该 Socket 内部操作产生的错误码，会写入传入缓冲区socketError
    // getsockopt返回0表示查询成功，-1查询失败
    {
        socketError = errno;
        log_error("Fail to locate errors on socket fd %d with errno %d",socket_->Getfd(),socketError);
        handleClose();
    }
    else
    {
        socketError = errno;
        log_error("socket fd %d failed with errno %d",socket_->Getfd(),socketError);
        handleClose();
    }
}

void TcpConnection::sendInLoop(const char* data, size_t len)
{
    if (state_==Disconnected) return;
    else if (len<=0) return;

    if (outputBuffer_.readableBytes()>0 || channel_->isWriting()==true) 
    // 只有输出缓冲为空且没有进行中的写关注时，才尝试直接写新数据
    {
        outputBuffer_.bufferAppend(len,const_cast<char*>(data));
    }
    else
    {
        int errors;
        int result=outputBuffer_.writeFd(socket_->Getfd(),&errors);
        errors=errno;
        if (result>0)
        {
            if (result<len)
            {
                outputBuffer_.bufferAppend(len-result,const_cast<char*>(data)+result);
                channel_->enableWriting();
            }
            else channel_->disableWriting();
        }
        else if (result==0) // 写入成功但是写入0个字节
        {
            outputBuffer_.bufferAppend(len,const_cast<char*>(data));
            channel_->enableWriting();
            sendInLoop(data,len);
            // 积压数据的处理由 handleWrite() 负责，不需要等下一次 sendInLoop()。
            // 只要写关注，每次epollwait就会可写时返回，触发handlewrite处理缓冲区
            return;
        }
        else
        {
            if (errors==EAGAIN||errors==EWOULDBLOCK)
            // 内核 TCP Send Buffer已经满了，非阻塞写操作无法写入任何数据
            {
                outputBuffer_.bufferAppend(len,const_cast<char*>(data));
                channel_->enableWriting();
                return;
            }
            else if (errors==EINTR) //中断重来
            {
                sendInLoop(data,len);
                return;
            }
            else
            {
                handleError();
            }
        }
    }
}

























