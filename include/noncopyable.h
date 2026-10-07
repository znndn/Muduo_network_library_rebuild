#pragma once
#include <iostream>

class noncopyable
{
    public:
    noncopyable (const noncopyable& other)=delete;
    noncopyable& operator= (const noncopyable other)=delete;

    protected:
    // 构造和析构一定要记得声明完建立
    noncopyable()
    {
    }
    ~noncopyable()
    {
    }
};