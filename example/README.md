# 用 nc 手动演示回声

示例只保留 `main`、`EchoServer` 包装和两个回调。无需 Python 客户端、命令行参数或额外的终端控制线程。代码在远程项目的 `example/testserver.cc`，可直接采用，不用补空。

## 启动与连接

两个终端都连接同一台远程 Ubuntu。先在服务器终端执行：

```bash
cd /home/ubuntu/projects/Muduo_network_library_rebuild
cmake -S . -B build
cmake --build build -j2
./build/testserver
```

看到 `EchoServer listening on 127.0.0.1:8080` 后，在另一个远程终端运行：

```bash
nc 127.0.0.1 8080
```

输入 `hello` 并回车。通常会看到两行：

```text
hello
hello
```

第一行是终端显示你自己的输入，第二行是服务器返回的回声。可以继续输入中文或其他文本。

`127.0.0.1` 是运行 nc 那台机器的回环地址，所以这里让两个终端都在远程 Ubuntu。示例只绑定这个本机地址，不需要配置公网端口。如果端口已被占用，先结束你自己之前启动的同端口服务。

## 用 Ctrl+C 观察断开

在 **nc 终端**按 Ctrl+C，结束客户端。服务器终端随后打印 `Connection DOWN`，服务器自身继续运行。重新运行 nc，仍可建立新连接并回显。

这时的路径是：客户端关闭 socket → 服务器发现 EOF 或相关关闭事件 → handleClose 通知 DOWN → TcpServer 安排表移除和 Channel 注销 → 最后一个强引用释放后 Socket 关闭。DOWN 表示逻辑断开，不保证打印这一刻全部异步清理已结束。

结束整个演示时，再在**服务器终端**按 Ctrl+C。它按 SIGINT 的默认行为终止进程；不会按 C++ 作用域逐层析构，也不经过本项目的 `TcpServer::stop()` 完成流程。这一操作不用于证明正常停机顺序。参见 [Linux signal 手册](https://man7.org/linux/man-pages/man7/signal.7.html)。

## 需要解释的代码只有这些关系

- main 创建 EventLoop、监听地址和 EchoServer，start 接通监听，再由 loop.loop 持续处理事件。固定使用两个额外 I/O 线程。
- EchoServer 是应用类，持有一个 TcpServer；网络库负责接入、分配连接和回收，应用注册回调决定收到数据后做什么。
- onConnection 只打印 UP/DOWN，不在这里手动删除连接。
- 进入 onMessage 时，handleRead 已把字节放入 Buffer。retrieveAllAsString 复制并消费可读区，send 原样发回；写不完的部分继续由已有 outputBuffer 和可写事件处理。
- 启动时忽略 SIGPIPE，是为了让对端断开后的写失败走库的返回值处理。它与 Ctrl+C 的 SIGINT 是不同信号，不需要为此新增信号处理模块。

onMessage 仍是按字节回显，没有新增按行拆包。nc 按回车发送输入，不表示 TCP 自带“一行一个消息”的边界。

这个入口用于展示连接、回声和客户端断开。此前少量多客户端联动结果仍是功能验证记录，不是压力测试，也不要求你阅读或解释当时的辅助脚本。
