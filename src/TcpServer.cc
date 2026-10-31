
#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "Logger.h"
#include <cassert>
#include <cerrno>
#include <utility>
#include <sys/socket.h>
#include <unistd.h>

static EventLoop* checkedLoop(EventLoop* loop)
{
    if (loop == nullptr)
        log_fatal("TcpServer needs a baseLoop");
    assert(loop->isInLoopThread());
    return loop;
}

static bool readLocalAddress(int sockfd, InetAddress* localAddr)
// 将sockfd的ip和port写入传入的Inet类localAddr
{
    sockaddr_in local{};
    socklen_t length = sizeof(local);

    if (getsockname(sockfd, reinterpret_cast<sockaddr*>(&local), &length) < 0)
    // int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    // 获取与某个套接字（Socket）关联的本地协议地址,通用强转为IPv4 专用的套接字地址
    {
        int error = errno;
        log_error("getsockname failed, fd=%d errno=%d", sockfd, error);
        return false;
    }

    localAddr->setSocketAddress(local);
    return true;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
                     const std::string& nameArg, Option option)
    : loop_(checkedLoop(loop)),
      ipPort_(listenAddr.toIPPort()),
      name_(nameArg),
      acceptor_(std::make_unique<Acceptor>(loop_, listenAddr,
                                         option == kReusePort)),
      threadPool_(std::make_unique<EventLoopThreadPool>(loop_, name_)),
      started_(false),
      nextConnId_(1),
      stopping_(false),
      stopped_(false),
      pendingCleanups_(0)
{
    acceptor_->setNewConnectionCallback(
        [this](int fd, const InetAddress& peerAddr) {
            newConnection(fd, peerAddr);
        });
}

TcpServer::~TcpServer()
{
    assert(loop_->isInLoopThread());

    if (started_ && !stopped_)
        log_fatal("TcpServer must finish stop before destruction");

    assert(connections_.empty() && pendingCleanups_ == 0);
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(loop_->isInLoopThread() && !started_);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::setThreadInitCallback(const ThreadInitCallback& cb)
{
    assert(loop_->isInLoopThread() && !started_);
    threadInitCallback_ = cb;
}

void TcpServer::setConnectionCallback(const ConnectionCallback& cb)
{
    assert(loop_->isInLoopThread() && !started_);
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback& cb)
{
    assert(loop_->isInLoopThread() && !started_);
    messageCallback_ = cb;
}

void TcpServer::start()
{
    assert(loop_->isInLoopThread());

    if (started_)
        return;
    started_ = true;

    threadPool_->start(threadInitCallback_); // 建立新的线程池并注册回调
    // 预先根据numThreads_数量创建多个空的线程（可能没有socketfd），
    // 只有EventLoopThread::threadFunc（在startloop调用链中）提供的各线程eventfd
    acceptor_->listen(); // 设置对于新连接的监听
}

void TcpServer::stop(StopCallback cb)
{
    assert(loop_->isInLoopThread());
    assert(started_);

    if (stopping_)
        return;

    stopping_ = true; // 新的fd不再添加，然后析构acceptor
    stopCallback_ = std::move(cb);

    loop_->queueInLoop([this]() 
    {
        acceptor_.reset(); 
        maybeFinishStop();
    });

    loop_->wakeup();
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    assert(loop_->isInLoopThread());

    if (stopping_)
    {
        close(sockfd);
        return;
    }

    InetAddress localAddr;
    if (readLocalAddress(sockfd, &localAddr)==false)
    // 获取自己的地址和端口进localaddr
    {
        close(sockfd);
        return;
    } // 后续close socket fd交给tcpconnection

    // 取名字
    std::string connName = name_ + "-" + ipPort_ + "#" + std::to_string(nextConnId_);

    nextConnId_++;

    // TODO 1：从 threadPool_ 选择一个 EventLoop*，局部变量名为 ioLoop。

    auto ioLoop = threadPool_->getNextLoop();

    // TODO 2：创建 TcpConnectionPtr conn，并把它存入 connections_[connName]。

    TcpConnectionPtr conn=std::make_unique<TcpConnection>(ioLoop,connName,sockfd,localAddr,peerAddr);

    connections_[connName]=conn;

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr& closing) 
    {
        removeConnection(closing);
    });

    // TODO 3：把 connectEstablished 交给 ioLoop；任务按值捕获 conn。
    ioLoop->runInLoop([conn]()
    {
        conn->connectEstablished();
    });
    
}

// handleclose 仅仅解除了channel的全部关注，然后应该是上层收到回调，调用removeConnection
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // TODO A：通过服务器的 loop_ 调用 runInLoop。
    // 任务按值保存 this 和 conn，执行 removeConnectionInLoop(conn)。
    loop_->runInLoop([this,conn]()
    {
        removeConnectionInLoop(conn);
    });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    assert(loop_->isInLoopThread());

    auto item = connections_.find(conn->name());
    if (item == connections_.end()) 
        return;  // 没找到说明已经移除，不重复安排清理。

    assert(item->second == conn);

    // TODO B：取得 conn 所属的 EventLoop* ioLoop，然后移除 item。

    pendingCleanups_++;  // 配套记账直接采用，只在 A 修改。
    auto ioLoop = conn->getLoop();
    connections_.erase(item);

    // TODO C：在 ioLoop 上 queueInLoop，lambda 按值保存 this、conn。
    // lambda 的内容依次是：
    // 1. conn->channelDistroy();
    // 2. reportCleanupDone();

    ioLoop->queueInLoop([this,conn]()
    {
        conn->channelDistroy();
        reportCleanupDone();
    });

}

void TcpServer::reportCleanupDone()
{
    loop_->queueInLoop([this]() 
    {
        assert(pendingCleanups_ > 0); // 如果-1之前就等于0了，那就有问题了
        pendingCleanups_--;
        maybeFinishStop();
    });
}

void TcpServer::maybeFinishStop()
{
    assert(loop_->isInLoopThread());// 确认在主线程baseloop 

    if (!stopping_ || stopped_ || acceptor_!=nullptr || !connections_.empty()|| pendingCleanups_ != 0)
        return;

    threadPool_.reset();
    stopped_ = true;
    StopCallback cb = std::move(stopCallback_);
    if (cb)
        cb();
}

// 1. A 标记 stopping，后来的新 fd 不再装配成连接。
// 2. A 在任务阶段销毁 Acceptor，取消监听 Channel 登记并关闭监听 socket。
// 3. 已有连接继续由各自 ioLoop 服务，等待客户端正常结束。
// 4. 连接表为空，并且所有已安排的 Channel 注销都向 A 确认完成后，A 回收线程池。
// 5. A 调用完成回调；应用这时可以让 baseLoop.quit，随后按作用域析构服务器与 baseLoop。




















