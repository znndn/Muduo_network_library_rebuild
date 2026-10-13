#include <string>
#include <vector>
#include "buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <algorithm>
#include <unistd.h>

Buffer::Buffer()
{   
    readerIndex=SpaceForProtHeader; // 相当于最开始写的位置（包括报头，如果有）
    writerIndex=SpaceForProtHeader;  // 最后写到的位置（后边都是空白）
    buffer.resize(SpaceForProtHeader+SpaceInitialize);
}

int Buffer::readableBytes()
{
    return writerIndex-readerIndex;
}
int Buffer::writableBytes()
{
    // 注意buffer是vector<char>，不能使用sizeof（整个类），而是.size，算法题写过很多遍了
    return buffer.size()-writerIndex;
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
    if (UsedLength<=readableBytes())
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

void Buffer::ensureNewStringSize(int ByteRequested)
{
    if (writableBytes()<ByteRequested) // 无须扩容也无须挪位置的情况
    {
        //啥也不做
    }
    else
    {
        MoveorExpend(ByteRequested); // 需要挪位置或者扩容的情况
    }
}

void Buffer::MoveorExpend(int size) // size字节的大小加入了buffer
{
        /**
     * | SpaceForProtHeader |xxx| reader | writer |                     // xxx标示reader中已读的部分
     * | SpaceForProtHeader | reader ｜          len          |
     **/
    if (writableBytes() + beforeHeaderBytes() < size + SpaceForProtHeader) 
    { // 等价于buffer.size()-writerIndex 和  size 的比较，主要是担心负数下溢才都加上8
        buffer.resize(writerIndex + size);
    }
    else 
    {
        // 把没读的（readerindex~writerindex移动到开头（报文头预留的空间SpaceForProtHeader之后）
        std::copy(&buffer[0] + readerIndex,
                  &buffer[0] + writerIndex,
                  &buffer[0] + SpaceForProtHeader);
        writerIndex = writerIndex - readerIndex + SpaceForProtHeader;
        readerIndex = SpaceForProtHeader;
    }
}

char* Buffer::beginWriteLoc()
{
    return &buffer[0] + writerIndex;
}

void Buffer::bufferAppend(int size,char* data)
{
    ensureNewStringSize(size);
    char* pointer=data;
    for (int i=0;i<size;i++)
    {
        buffer.push_back(*pointer);
        pointer++;
    }
    writerIndex+=size;
    delete pointer;
}



// 从fd上读取数据，最终存入buffer中
// 使用多一个缓冲区（避免buffer由于太小需要不停扩容，影响性能），先存入新的缓冲区中
// 再逐步存入buffer，避免readv系统调用阻塞

// 使用iovec结构体和readv协助：
// 在标准实现中，struct iovec 的结构通常如下：
// struct iovec {
//     void  *iov_base;  // 指向内存缓冲区的起始地址 (Buffer pointer)
//     size_t iov_len;   // 缓冲区的长度，单位为字节 (Length in bytes)
// };

int Buffer::readFd(int fd, int *saveErrno)
{
    char AuxBuffer[65536];
    struct iovec vec[2];
    vec[0].iov_base=&buffer[0]+writerIndex;
    vec[0].iov_len=buffer.size()-writerIndex;
    vec[1].iov_base=&AuxBuffer[0];
    vec[1].iov_len=65536;

    int iovcnt;
    int originalFreeSpace=writableBytes();
    // 这里要记得提前保存，readv之后就变成（可能）0了
    if (originalFreeSpace<65536) iovcnt=2;
    else iovcnt=1;

    const struct iovec* iov=vec;
    int result=readv(fd, &vec[0], iovcnt);
// readv(int fd, const struct iovec *iov, int iovcnt)：
// 从文件描述符 fd 读取数据，存入 iovcnt 个 iovec 指向的缓冲区中

    if (result==-1) //err
    {
        *saveErrno=errno;
    }
    // 成功时result就是读、写的总字节数，这里是不需要auxBuffer的情况
    else if (result<=originalFreeSpace) 
    {
        writerIndex+=result;
    }
    else // buffer不够大，先存入buffer，再存入AuxBuffer
    {
        writerIndex=buffer.size();
        // 已经填满buffer，修改writerindex
        bufferAppend(result-originalFreeSpace,&AuxBuffer[0]);
    }
    return result;
}

int Buffer::writeFd(int fd, int *saveErrno)
{
    int result=::write(fd, MsgStartLoc(), readableBytes());
    if (result==-1) //err
    {
        *saveErrno=errno;
    }
    return result;
}















































































