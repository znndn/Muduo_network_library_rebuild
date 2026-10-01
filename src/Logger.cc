#include <iostream>
#include "noncopyable.h"
#include "Logger.h"
#include "timestamp.h"

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
    std::string prefVar;
    switch (logLevel)
    {
    case 1:
        prefVar="info ";
        break;
    case 2:
        prefVar="error ";
        break;
    case 3:
        prefVar="fatal ";
        break;
    case 4:
        prefVar="debug";
        break;
    default:
        break;
    }
    std::cout<<prefVar<<Timestamp::now().toString()<<" : "<<std::endl;
}

// ##__VA_ARGS__ 用于各种括号之中，用于没有这个参数时把前边的逗号删除
// do while 0 相当于只做一遍，但是因为前面是define，所以一定要让它可以被作为整体识别

#define log_info(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::info);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,"%s",logMessage,##VA_ARGS);  \ 
        logLoc.log(buffer); \
    } while (0)  

#define log_error(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::error);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,"%s",logMessage,##VA_ARGS);  \ 
        logLoc.log(buffer); \
    } while (0)  

#define log_debug(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::debug);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,"%s",logMessage,##VA_ARGS);  \ 
        logLoc.log(buffer); \
    } while (0)  

#define log_fatal(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::fatal);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,"%s",logMessage,##VA_ARGS);  \ 
        logLoc.log(buffer); \
        exit(-1); \
    } while (0)  

























