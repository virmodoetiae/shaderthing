#ifndef V_MACROS_H
#define V_MACROS_H

#define DELETE_COPY(cName)                                                  \
    cName(const cName&)=delete;                                             \
    cName& operator=(const cName&)=delete;

#define DELETE_COPY_MOVE(cName)                                             \
    cName(const cName&)=delete;                                             \
    cName& operator=(const cName&)=delete;                                  \
    cName(cName&&)=delete;                                                  \
    cName& operator=(cName&&)=delete;

#define DELETE_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete ptr; ptr=nullptr;
#define DELETE_ARRAY_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete[] ptr; ptr=nullptr;

// Use TO_STRING instead of this, this is just a helper
#define _TO_STRING(x) #x

#define TO_STRING(x) _TO_STRING(x)

#endif