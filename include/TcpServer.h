
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "noncopyable.h"
#include "CallBacks.h"

// TCPserver 只是将Acceptor 线程池 Tcpconnection组装起来，
// 只有挂在了Acceptor的Reactor是mainReactor

class Acceptor;
class EventLoop;
class EventLoopThreadPool;
class InetAddress;

class TcpServer : noncopyable
{
public:

    using ThreadInitCallback = std::function<void(EventLoop*)>;
    using StopCallback = std::function<void()>;

    enum Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
              const std::string& nameArg, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadNum(int numThreads);  // 设置线程池线程数
    void setThreadInitCallback(const ThreadInitCallback& cb);
    // 设置新eventloop 在loop()之前要执行的回调内容
    void setConnectionCallback(const ConnectionCallback& cb);
    // 设置建立新连接和销毁新连接需要调用的回调
    void setMessageCallback(const MessageCallback& cb);
    // 新数据已被读入缓冲区(调用handleread之后）需要调用的回调

    void start();
    void stop(StopCallback cb = StopCallback());

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);

    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    // 移除连接表记录

    // 单元 4 直接提供；单元 3 会先说明使用它们的前提。
    void reportCleanupDone();
    // 子线程调用的回调方法，让主线程减少pendingCleanups_数量并检查是否可以停机
    void maybeFinishStop();

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;
    // 在main中创建的Eventloop即为baseloop，后续传给Acceptor
    const std::string ipPort_;
    const std::string name_;
    // 自己（服务器本端）的名字和端口

    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    ThreadInitCallback threadInitCallback_;

    bool started_;

    std::uint64_t nextConnId_;
    // 链接计数，在 TcpServer::newConnection中取新名字
    
    ConnectionMap connections_;
    // 提供对于多个TcpConnection shareptr的强引用

    // 下列字段均只由 A 访问；没有让 B 直接修改计数。
    bool stopping_;// 停止新连接
    bool stopped_; // 回收完成

    std::size_t pendingCleanups_;
    // 已经安排、尚未向 A 确认完成的 Channel 清理数，需要B调用reportCleanupDone来减少
    StopCallback stopCallback_;
    // tcpserver完成全部停止之后向main()的回调通知
};














