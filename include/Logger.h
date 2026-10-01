#pragma once

#include <iostream>
#include "noncopyable.h"

class Logger: noncopyable
{
    private:
    static Logger& getLogger();
    Logger()=default;
    ~Logger()=default;
    int logLevel;
    enum LogLevel
    {
        info,error,fatal,debug
    };

    public:
    void setLogLevel(int level);
    void log(std::string msg);

    Logger& operator= (const Logger& other) = delete;
    Logger& operator= (const Logger&&) = delete;

    Logger (const Logger& other) = delete;
    Logger (const Logger&&) = delete;

};