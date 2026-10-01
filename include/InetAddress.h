#include <iostream>
#include <netinet/in.h>

class InetAddress
{
    private:
    sockaddr_in sockaddr;
    public:
    explicit InetAddress(uint16_t port=0,std::string ip="127.0.0.1");
    // 调用方如果没传这个参数，编译器就自动帮你填上等号右边的值
    explicit InetAddress(sockaddr_in &sockaddr);
    std::string toIP() const;
    std::string toIPPort() const;
    uint16_t toPort() const;
    const sockaddr_in *getSocketAddress() const;
    void setSocketAddress(const sockaddr_in &addr);
};