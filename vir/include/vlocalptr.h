#ifndef V_LOCAL_PTR_H
#define V_LOCAL_PTR_H

#include <iostream>

namespace vir
{

// Class to manage a local pointer to a resource, i.e., the pointed resource is
// destroyed when the LocalPtr goes out of scope
template<class T>
class LocalPtr
{
protected:

    // Pointed resource
    T* ptr_;

    // 
    LocalPtr(const LocalPtr&) = delete;
    LocalPtr(LocalPtr&&) = delete;
    LocalPtr& operator=(const LocalPtr&) = delete;
    LocalPtr& operator=(LocalPtr&&) = delete;

public:

    LocalPtr(T* ptr) : ptr_(ptr)
    {
        #if DEBUG
        std::cout<< "LocalPtr constructor" << std::endl; 
        #endif
    }

    // Destructor needs to detroy the managed instance as well
    ~LocalPtr()
    {   
        delete ptr_;
        ptr_ = nullptr;
        #if DEBUG 
        std::cout<< "LocalPtr destroyed" << std::endl; 
        #endif
    }

    //
    T* get(){return ptr_;}
};

}

#endif