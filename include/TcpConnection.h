#include <iostream>
#include <map>
#pragma once

class Buffer;

class TcpConnection
{
    public:
    // 第一个参数是fd，通过fd确定该buffer是否活着
    std::map<int, Buffer> inputBufferMap;
    std::map<int, Buffer> onputBufferMap;
};
