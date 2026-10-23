
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

TcpConnection::TcpConnection(
                EventLoop* loop,
                const std::string& nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr):
                
                loop_(loop),name_(nameArg),
                socket_(std::make_unique<Socket>(sockfd)),
                localAddr_(localAddr),
                peerAddr_(peerAddr)
{
    setstate(Connecting);
}

TcpConnection::~TcpConnection()
{
    setstate(Disconnecting);
}



























