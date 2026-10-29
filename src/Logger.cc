#include <iostream>
#include <mutex>
#include "noncopyable.h"
#include "Logger.h"
#include "timestamp.h"

thread_local int Logger::logLevel = Logger::info;
// 设置为info级别的logLevel默认

Logger::Logger()
{
    logLevel=0;
}

Logger& Logger::getLogger()
{
    static Logger newLogger;
    return newLogger;
}

void Logger::setLogLevel(int level)
{
    logLevel=level;
}

void Logger::log(std::string msg)
{
    static std::mutex outputMutex;
    // 这里的 mutex 是各线程共同使用的一把锁，
    // 让本函数一次完整输出不与另一次日志拼在一起。锁必须是共享的
    std::lock_guard<std::mutex> lock(outputMutex);

    std::string prefVar;
    switch (logLevel)
    {
    case 0:
        prefVar="info ";
        break;
    case 1:
        prefVar="error ";
        break;
    case 2:
        prefVar="fatal ";
        break;
    case 3:
        prefVar="debug";
        break;
    default:
        break;
    }
    std::cout<<prefVar<<Timestamp::now().toString()<<" : "<<msg<<std::endl;
}

















