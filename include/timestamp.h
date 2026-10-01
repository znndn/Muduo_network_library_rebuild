#pragma once
#include <iostream>
#include <time.h>
#include <chrono>

class Timestamp
{
    public:
    Timestamp();
    explicit Timestamp(long microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

    private:
    long microSecondsSinceEpoch_;
};