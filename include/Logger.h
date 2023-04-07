#pragma once
#include <string>
#include "noncopyable.h"
/*
��־����INFO ERROR FATAL DEBUG
*/
//LOG_INFO(%s %d, agr1, arg2)
#define LOG_INFO(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);\
        logger.log(buf); \
    } while(0)
#define LOG_ERROR(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);\
        logger.log(buf); \
    } while(0)
#define LOG_FATAL(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);\
        logger.log(buf); \
        exit(-1);\
    } while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);\
        logger.log(buf); \
    } while(0)
#else
#define LOG_DEBUG(LogmsgFormat, ...)
#endif

enum LoggerLevel {
    INFO, //��ͨ��־��Ϣ
    ERROR,  //������־��Ϣ
    FATAL,  //core dump��Ϣ
    DEBUG  //������Ϣ
};

class Logger
{
public:
    static Logger& Instance(); //��ȡ��־ʵ������
    void setLogLevel(int level); //������־����
    void log(std::string msg); //д��־
private:
    Logger() = default;
    int level_; //��־����

};