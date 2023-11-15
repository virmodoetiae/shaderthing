#ifndef V_GLOBALPTR_H
#define V_GLOBALPTR_H

#include <iostream>

namespace vir
{

// Class to manage a global pointer to a resource
template<class T>
class GlobalPtr
{
protected:
    
    // Protected default constructor
    GlobalPtr(){}
    
    // Destructor needs to detroy the managed instance as well
    ~GlobalPtr()
    {   
        // Only delete resource if it still exists (i.e., if this globalPtr was
        // not directly managing/owning it
        if (GlobalPtr<T>::instance() != nullptr)
        {
            delete GlobalPtr<T>::instance(); // Delete managed resource
            GlobalPtr<T>::instance(nullptr, true);  // Free pointer to managed
                                                    //resource
        }
    }

    GlobalPtr(const GlobalPtr&) = delete;
    GlobalPtr(GlobalPtr&&) = delete;
    GlobalPtr& operator=(const GlobalPtr&) = delete;
    GlobalPtr& operator=(GlobalPtr&&) = delete;

public:

    static T* instance(T* ptr = nullptr, bool override=false)
    {
        static GlobalPtr<T> globalPtr; // To make sure destructor will be called
        static T* instance;
        if ((instance == nullptr && ptr != nullptr) || override)
            instance = ptr;
        return instance;
    }
    
    // If the global pointer is valid, copy it to ptr and return true
    static bool valid(T*& ptr)
    {
        T* instance = GlobalPtr<T>::instance();
        if (instance == nullptr)
            return false;
        ptr = instance;
        return true;
    }
};

}

#endif