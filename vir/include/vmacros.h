#ifndef V_MACROS_H
#define V_MACROS_H

#define DELETE_COPY_MOVE(class)                                             \
    class(const class&)=delete;                                             \
    class& operator=(const class&)=delete;                                  \
    class(class&&)=delete;                                                  \
    class& operator=(class&&)=delete;

#define DELETE_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete ptr; ptr=nullptr;
#define DELETE_ARRAY_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete[] ptr; ptr=nullptr;

// Use TO_STRING instead of this, this is just a helper
#define _TO_STRING(x) #x

#define TO_STRING(x) _TO_STRING(x)

#endif