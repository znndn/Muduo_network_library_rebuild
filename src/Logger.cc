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
    std::cout<<prefVar<<Timestamp::now().toString()<<" : "<<msg<<std::endl;
}

















