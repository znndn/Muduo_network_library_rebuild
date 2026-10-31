#include "TcpServer.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "Logger.h"
#include "buffer.h"
#include "timestamp.h"

#include <csignal>
#include <string>

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        : server_(loop, addr, name)
    {
        server_.setConnectionCallback([this](const TcpConnectionPtr& conn) {
            onConnection(conn);
        });
        server_.setMessageCallback(
            [this](const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
                onMessage(conn, buf, time);
            });
        server_.setThreadNum(2);
    }

    void start()
    {
        server_.start();
    }

private:
    // 建立或逻辑断开时通知应用；连接回收仍由网络库推进。
    void onConnection(const TcpConnectionPtr& conn)
    {
        log_info("Connection %s : %s",
                 conn->connected() ? "UP" : "DOWN",
                 conn->peerAddress().toIPPort().c_str());
    }

    // 进入时数据已经在 Buffer 中；取出并消费，再原样发回。
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
    }

    TcpServer server_;
};

int main()
{
    // 对端断开时，让 write 的错误返回给库处理，避免 SIGPIPE 终止服务器。
    if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;

    EventLoop loop;
    InetAddress addr(8080, "127.0.0.1");
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    log_info("EchoServer listening on %s", addr.toIPPort().c_str());
    loop.loop();
}
