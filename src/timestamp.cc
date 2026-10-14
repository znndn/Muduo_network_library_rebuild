
#include <iostream>
#include <time.h>
#include <chrono>
#include <timestamp.h>

Timestamp::Timestamp()
{
    SecondsSinceEpoch_=0;
}

Timestamp::Timestamp(long SecondsSinceEpoch)
    :SecondsSinceEpoch_(SecondsSinceEpoch)
{
}

Timestamp Timestamp::now()
{
    // 整个项目都避免new和delete，违反raii
    // 不new的办法就是去构造新的（作为局部变量），然后直接返回
    Timestamp curr(time(NULL));
    return curr;
}

std::string Timestamp::toString() const
{
    char arr[128]={0};
    auto curr=localtime(&SecondsSinceEpoch_);
    snprintf(arr,128,"%4d/%02d/%02d %02d:%02d:%02d", 
        curr->tm_year+1900,
        curr->tm_mon+1,
        curr->tm_mday,
        curr->tm_hour,
        curr->tm_min,
        curr->tm_sec);
    std::string result(arr);
    return result;
}

























