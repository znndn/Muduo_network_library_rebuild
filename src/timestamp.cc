
#include <iostream>
#include <time.h>
#include <chrono>
#include <timestamp.h>

Timestamp::Timestamp()
{
    microSecondsSinceEpoch_=0;
}

Timestamp::Timestamp(long microSecondsSinceEpoch)
    :microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

Timestamp Timestamp::now()
{
    Timestamp* current = new Timestamp(std::time(nullptr));
    return *current;
}

std::string Timestamp::toString() const
{
    char arr[128]={0};
    auto curr=localtime(&microSecondsSinceEpoch_);
    snprintf(arr,128,"%d/%d/%d:%d:%d:%d", 
        curr->tm_year+1900,
        curr->tm_mon+1,
        curr->tm_mday,
        curr->tm_hour,
        curr->tm_min,
        curr->tm_sec);
    std::string result(arr);
    return result;
}

























