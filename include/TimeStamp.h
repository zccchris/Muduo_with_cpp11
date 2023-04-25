#pragma once
//后续改成chrono版本

#include <string>



class TimeStamp {
public:
    TimeStamp();
    explicit TimeStamp(int64_t microSecondSinceEpoch); //禁止隐式转换
    static TimeStamp now(); //返回当前时间
    std::string toString() const;

private:
    int64_t microSecondSinceEpoch_; //muduo中用了一个int64_t作为当前时间，单位为微妙
};
