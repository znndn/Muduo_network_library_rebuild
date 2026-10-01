#include <iostream>
#include <string>
#include <vector>
#include <buffer.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

Buffer::Buffer()
{   
    readerIndex=SpaceForProtHeader; // 相当于最开始写的位置（包括报头，如果有）
    writerIndex=SpaceForProtHeader;  // 最后写到的位置（后边都是空白）
}

int Buffer::readableBytes()
{
    return writerIndex-readerIndex;
}
int Buffer::writableBytes()
{
    return sizeof(buffer)-writerIndex;
}
int Buffer::beforeHeaderBytes()
{
    return readerIndex;
}

char* Buffer::MsgStartLoc()
{
    auto BufferLoc=&buffer[0];
    return BufferLoc + readerIndex;
}

void Buffer::useData(int UsedLength)
{
    if (UsedLength<=writerIndex)
    {
        readerIndex+=UsedLength;
    }
    else
    {
        useAllData();
    }
}
void Buffer::useAllData()
{
    readerIndex=SpaceForProtHeader;
    writerIndex=SpaceForProtHeader;
}

void Buffer::createWritingSpace(int ByteRequested)
{
    if (buffer.size()-writerIndex<ByteRequested) // 不用挪位置和扩容 直接加
    {
        buffer.resize(writerIndex+ByteRequested);
    }
    else
    {
        stringAddIntoBuffer(ByteRequested); // 需要挪位置或者扩容的情况
    }
}

void Buffer::stringAddIntoBuffer(int size)
{
        /**
     * | kCheapPrepend |xxx| reader | writer |                     // xxx标示reader中已读的部分
     * | kCheapPrepend | reader ｜          len          |
     **/
    if (writableBytes() + beforeHeaderBytes() < size + SpaceForProtHeader) 
    { // 等价于buffer.size()-writerIndex 和  size 的比较，主要是担心负数下溢才都加上8
        buffer.resize(writerIndex + size);
    }
    else 
    {
        // 把没读的移动到开头（为了给报文头预留的空间之后）
        std::copy(&buffer[0] + readerIndex,
                  &buffer[0] + writerIndex,
                  &buffer[0] + SpaceForProtHeader);
        readerIndex = SpaceForProtHeader;
        writerIndex = readerIndex + readableBytes();
    }
}

char* Buffer::beginWriteLoc()
{
    return &buffer[0] + writerIndex;
}

// 从fd上读取数据，最终存入buffer中
// 使用多一个缓冲区（避免buffer由于太小需要不停扩容，影响性能），先存入新的缓冲区中
// 再逐步存入buffer，避免readv系统调用阻塞（append全称没有系统调用，不会阻塞）
int Buffer::readFd(int fd, int *saveErrno)
{
    char AuxBuffer[65536];
    struct iovec vec[2];
    vec[0].iov_base=&buffer[0]+writerIndex;
    vec[0].iov_len=sizeof(buffer)-writerIndex;
    vec[1].iov_base=&AuxBuffer[0];
    vec[1].iov_len=65536;

    int iovcnt;
    if (buffer.size()>=65536) iovcnt=1;
    else iovcnt=2;

    const struct iovec* iov=vec;
    int result=readv(fd, &vec[0], iovcnt);

    if (result==-1) //err
    {
        *saveErrno=errno;
    }
    // 成功时result就是读、写的总字节数
    else if (iovcnt==1) 
    {
        writerIndex+=result;
    }
    else // buffer不够大，先存入AuxBuffer，再存入buffer
    {
        writerIndex=buffer.size();
        stringAddIntoBuffer(result-writableBytes());
        std::copy(&AuxBuffer[0], &AuxBuffer[0]+result-vec[0].iov_len, &buffer[0]+writerIndex);
        writerIndex+=result-vec[0].iov_len;
    }
    return result;
}

int Buffer::writeFd(int fd, int *saveErrno)
{
    int result=::write(fd, &buffer[0]+readerIndex, readableBytes());
    if (result==-1) //err
    {
        *saveErrno=errno;
    }
    else
    {
        readerIndex+=result;
    }
    return result;
}















































































