#pragma once

#define DELETE_COPY_MOVE(class)                                             \
    class(const class&)=delete;                                             \
    class& operator=(const class&)=delete;                                  \
    class(class&&)=delete;                                                  \
    class& operator=(class&&)=delete;

#define DELETE_IF_NOT_NULLPTR(ptr) if (ptr != nullptr) delete ptr;