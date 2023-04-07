#pragma once 


/***
*   ���������������������������ǲ��ܿ�������͸�ֵ.
***/
class noncopyable{
    
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete; //���Ҫ��������ֵ�Ļ��ͷ���noncopyable&����Ȼ�ͷ���void
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};