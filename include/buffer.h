#include <iostream>
#include <string>
#include <vector>
#pragma once

constexpr int SpaceForProtHeader = 8;
constexpr int SpaceInitialize = 1024;

class Buffer
{
    private:
    int readerIndex; //包括报头，此index之前是读完可以扔掉的内容，之后是未读内容
    int writerIndex; //从这里开始写新的内容
    std::vector<char> buffer;

    public:
    Buffer(){};
    ~Buffer(){};

    int readableBytes(); // 未读长度
    int writableBytes();
    int beforeHeaderBytes(); // 返回报文头预留长度的剩余空间

    // 返回报文头（如果有）的起始地址
    char* MsgStartLoc();

    void useData(int UsedLength); // 输入新读取的长度
    void useAllData(); // 重置回开始构造的样子

    void createWritingSpace(int ByteRequested);
    void stringAddIntoBuffer(int size);
    char* beginWriteLoc();


    int readFd(int fd, int *saveErrno);
    // 返回readv的结果，-1表示失败，其他表示读到的字节数
    int writeFd(int fd, int *saveErrno);


};














