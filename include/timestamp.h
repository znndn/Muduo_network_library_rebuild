#pragma once
#include <iostream>
#include <time.h>
#include <chrono>

class Timestamp
{
    public:
    Timestamp();
    explicit Timestamp(long SecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

    private:
    long SecondsSinceEpoch_;
};