#pragma once

#include <iostream>
#include "noncopyable.h"

class Logger: noncopyable
{
    private:
    
    Logger();
    // 这里最好初始化loglevel
    ~Logger()=default;
    static thread_local int logLevel;
    // 让每个线程持有一份loglevel副本，避免互相覆盖

    public:

    static Logger& getLogger();

    enum LogLevel
    {
        info,error,fatal,debug
    };

    void setLogLevel(int level);
    void log(std::string msg);

    Logger& operator= (const Logger& other) = delete;
    Logger& operator= (const Logger&&) = delete;

    Logger (const Logger& other) = delete;
    Logger (const Logger&&) = delete;

};


// 会被别的文件引用的宏一定要定义在头文件中！！
// ##__VA_ARGS__ 用于各种括号之中，用于没有这个参数时把前边的逗号删除
// do while 0 相当于只做一遍，但是因为前面是define，所以一定要让它可以被作为整体识别

#define log_info(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::info);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/\
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,logMessage,##__VA_ARGS__);  \
        logLoc.log(buffer); \
    } while (0)  

#define log_error(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::error);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,logMessage,##__VA_ARGS__);  \
        logLoc.log(buffer); \
    } while (0)  

#define log_debug(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::debug);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,logMessage,##__VA_ARGS__);  \
        logLoc.log(buffer); \
    } while (0)  

#define log_fatal(logMessage,...) \
    do  \
    {   \
        Logger& logLoc=Logger::getLogger();  \
        logLoc.setLogLevel(Logger::LogLevel::fatal);   \
        /*注意上面这种，类名::可选的enum名称::具体的枚举元素*/                \
        char buffer[1024]={0};  \
        snprintf(&buffer[0],1024,logMessage,##__VA_ARGS__);  \
        logLoc.log(buffer); \
        exit(-1); \
    } while (0)  

// 注意这里的区别，log_fatal会结束程序，别的不会。






