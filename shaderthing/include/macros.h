#include <chrono>

#pragma once

#define DELETE_COPY_MOVE(class)                                             \
    class(const class&)=delete;                                             \
    class& operator=(const class&)=delete;                                  \
    class(class&&)=delete;                                                  \
    class& operator=(class&&)=delete;

#define DELETE_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete ptr; ptr=nullptr;

// Use TO_STRING instead of this, this is just a helper
#define _TO_STRING(x) #x

#define TO_STRING(x) _TO_STRING(x)

#define START_TIMING                                                        \
    static double __time = 0;                                               \
    auto __startTime = std::chrono::high_resolution_clock::now();

#define END_TIMING                                                          \
    auto __endTime = std::chrono::high_resolution_clock::now();             \
    double __newTime = double((__endTime-__startTime).count())/1000.0;      \
    __time = 0.005*__newTime+.995*__time;                                   \
    std::cout << __time << " us" << std::endl;
