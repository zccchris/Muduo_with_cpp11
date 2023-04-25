#pragma once
//�����ĳ�chrono�汾

#include <string>



class TimeStamp {
public:
    TimeStamp();
    explicit TimeStamp(int64_t microSecondSinceEpoch); //��ֹ��ʽת��
    static TimeStamp now(); //���ص�ǰʱ��
    std::string toString() const;

private:
    int64_t microSecondSinceEpoch_; //muduo������һ��int64_t��Ϊ��ǰʱ�䣬��λΪ΢��
};
