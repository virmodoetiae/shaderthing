#include <memory>

#ifndef V_POINTERS_H
#define V_POINTERS_H

//----------------------------------------------------------------------------//

// Smart ptr that enables treating any class instance like a singleton. 
// Ownership is transferred
template<class T>
class GlobalPtr
{
protected:

    static std::unique_ptr<T> ptr_;

    GlobalPtr() = delete;
    GlobalPtr(const GlobalPtr&) = delete;
    GlobalPtr(GlobalPtr&&) = delete;
    GlobalPtr& operator=(const GlobalPtr&) = delete;
    GlobalPtr& operator=(GlobalPtr&&) = delete;

public:

    // Initialize with a specific instance and take ownership
    static T* set(T* ptr)
    {
        if (!ptr_ && ptr)
            ptr_.reset(ptr);
        return ptr_.get();
    }

    // Initialize with a specific instance and take ownership
    static T* set(std::unique_ptr<T>&& ptr)
    {
        if (!ptr_ && ptr)
            ptr_ = ptr;
        return ptr_.get();
    }

    // Get a naked pointer to the internally managed object
    static T* get() {return ptr_.get();}
    
    // Returns true if the internally managed object exists
    static bool valid() {return ptr_ != nullptr;}
};

template<class T>
std::unique_ptr<T> GlobalPtr<T>::ptr_ = nullptr;

//----------------------------------------------------------------------------//

// Smart ptr base class for UniquePtr and WeakPtr
template<typename T>
class Ptr
{
public:

    Ptr(const Ptr&) = delete;
    Ptr& operator=(const Ptr&) = delete;
    Ptr(Ptr&& other) = delete;
    Ptr& operator=(Ptr&& other) = delete;

    virtual ~Ptr(){}

    // Returns true if this Ptr is a UniquePtr, false if a WeakPtr
    virtual bool owner() const = 0;

    // Returns true if the owner is still valid. Always true for UniquePtr
    virtual bool valid() const = 0;

    // Get a naked pointer to the internally managed object
    virtual T* get() const = 0;

    // Get a naked pointer to the internally managed object
    T* operator->() const { return get(); }

    bool operator==(const T* other) const {return this->get()==other;}
    bool operator==(const Ptr& other) const {return this->get()==other.get();}
    bool operator!=(const T* other) const {return !(*this)==other;}
    bool operator!=(const Ptr& other) const {return !(*this)==other;}
};

//----------------------------------------------------------------------------//

// Smart ptr to an object owned by another UniquePtr. Enables validity checks
// on the lifetime of the onwer (i.e., is the owner is alive?) via the .valid()
// method
template<typename T>
class WeakPtr : public Ptr<T>
{
private:

    T* ptr_;
    std::weak_ptr<void> valid_;

public:

    WeakPtr(T* p, std::shared_ptr<void> flag) : ptr_(p), valid_(flag) {}

    // WeakPtr are never owners
    bool owner() const override {return false;}

    // Returns false if the owner was destroyed
    bool valid() const override{return !valid_.expired();}
    
    // Returns ptr to the owner or nullptr if the owner was destroyed
    T* get() const override {return valid() ? ptr_ : nullptr;}
};

//----------------------------------------------------------------------------//

// Smart ptr that fundamentally acts like std::unique_ptr<T> but which allows
// taking weak ptrs to it
template<typename T>
class UniquePtr : public Ptr<T>
{
private:

    std::unique_ptr<T> ptr_;
    std::shared_ptr<void> valid_;

public:

    UniquePtr() : 
        ptr_(nullptr), 
        valid_(nullptr) 
        {}

    UniquePtr(T* ptr) : 
        ptr_(ptr), 
        valid_(ptr ? std::make_shared<void>() : nullptr) 
        {}

    UniquePtr(UniquePtr&& other) = default;
    UniquePtr& operator=(UniquePtr&& other) = default;
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;
    
    ~UniquePtr() {valid_.reset();}

    // UniquePtrs are always owners
    bool owner() const override {return true;}

    // UniquePtrs are always valid
    bool valid() const override {return true;}

    // Get naked ptr to internally managed object
    T* get() const override { return ptr_.get(); }

    // Get ref to internally managed object
    T& operator*() { return *ptr_; }

    // Reset to nullptr
    void reset()
    {
        if (ptr_) 
        {
            valid_.reset(); // Invalidate all existing WeakPtrs to this
            ptr_.reset();
        }
    }
    
    // Reset to new pointer
    void reset(T* ptr) 
    {
        if (ptr_.get() != ptr) 
        {
            valid_.reset();  // Invalidate all existing WeakPtrs to this
            ptr_.reset(ptr);
            valid_ = ptr ? std::make_shared<void>() : nullptr;
        }
    }

    // Return a weak-like ptr to safely access and check for the existence 
    // of the internally managed object
    WeakPtr<T> getWeak() {return WeakPtr<T>(ptr_.get(), valid_);}
};

#endif