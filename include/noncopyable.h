#pragma once
#include <iostream>

// 删除拷贝构造和拷贝赋值，来达到禁止复制（但是不限制单例模式）的效果。
// 同时将构造和析构函数设为protected，让派生类隐式调用，但是不能拿

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