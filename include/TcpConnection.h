
#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include "noncopyable.h"
#include "InetAddress.h"
#include "CallBacks.h"
#include "buffer.h"
#include "timestamp.h"

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
// 多重继承，它允许一个被 std::shared_ptr 管理的对象，
// 在其成员函数内部安全地获取指向自身的 std::shared_ptr（通过调用 shared_from_this()）

// 连接对象tcpconnection每个实例表示和客户端的一个链接，通过该shareptr控制该实例的生命
// 不同位置需要短暂延长的是同一个连接对象的寿命
{
public:
    TcpConnection(EventLoop* loop,
                  const std::string& nameArg,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();
    // 先销毁tcpconnection，再销毁eventloop

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == Connected; }

    void send(const std::string& buf);
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,
        size_t highWaterMark)
    {
        assert(highWaterMark > 0);
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void connectEstablished();
    void channelDistroy();
    // 在正确线程解除 Channel 管理关系

private:
    enum StateE {
        Disconnected, 
        Connecting,  //  对象正在完成装配，尚未启用连接读关注
        Connected,  // 已建立对象关联并启用读
        Disconnecting // 已请求停止新发送
    };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    // 执行自身 channel 的回调函数，实际上执行的就是上述函数
    // 会在 tcp connection 的构造函数中绑定。

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    // 控制是否继续监听可读事件
    // 在触发高水位回调缓冲区积压严重的时候，可以设为 false。

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    // TCP 连接状态改变时的通知回调（建立成功 或 断开连接）
    MessageCallback messageCallback_; 
    // 新数据已被读入缓冲区(调用handleread之后）的通知回调(持有的channel负责具体逻辑)

    WriteCompleteCallback writeCompleteCallback_;
    // 应用层发送缓冲区（outputBuffer_）中的数据
    // 全部写入内核发送缓冲区时的通知回调（即“写完成”回调）。
    HighWaterMarkCallback highWaterMarkCallback_;

    CloseCallback closeCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};