#include <InetAddress.h>
#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

InetAddress::InetAddress(uint16_t port,std::string ip)
{
    std::memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family=AF_INET;
    sockaddr.sin_port=htons(port); 
    //用于发送的port，在发送方要 htons（host→network，短整型 16 位），小端->大端
    //接收方拿到线上那串字节后，要 ntohs（network→host）才能在算术运算里用
    // 会进报文头的网络参数要求大端存储，而x64机器是小端存储，因此进出必须做转换才行
    sockaddr.sin_addr.s_addr=::inet_addr(ip.c_str());
    // sin_addr 是 struct in_addr，内部是个 union，
    // s_addr 是其中那个 32 位无符号整数成员（类型 in_addr_t），它才是真正存地址的地方。
    // inetaddr最后一步自带了htons
}

InetAddress::InetAddress(sockaddr_in &sockaddr)
{
    this->sockaddr=sockaddr;
}

std::string InetAddress::toIP() const
{
    char arr[64]={0};
    inet_ntop(AF_INET,&sockaddr.sin_addr,&arr[0],sizeof(arr));
    return arr;
}

std::string InetAddress::toIPPort() const
{
    char arr[64]={0};
    inet_ntop(AF_INET,&sockaddr.sin_addr,&arr[0],sizeof(arr));
    uint16_t port=ntohs(sockaddr.sin_port);
    int ipv4Size=strlen(arr);
    arr[ipv4Size]=':';
    sprintf(&arr[ipv4Size+1],"%u",port);
    return arr;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(sockaddr.sin_port);
}

const sockaddr_in* InetAddress::getSocketAddress() const
{
    return &sockaddr;
}

void InetAddress::setSocketAddress(const sockaddr_in &addr)
{
    sockaddr=addr;
}



























