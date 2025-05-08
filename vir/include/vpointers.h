#include <memory>

#ifndef V_POINTERS_H
#define V_POINTERS_H

//----------------------------------------------------------------------------//

namespace vir
{

// Forward declarations
template<typename T>
class Ptr;
template<typename T>
class WeakPtr;
template<typename T>
class EnableWeakFromThis;
template<typename T>
class UniquePtr;

// Smart ptr base class for UniquePtr and WeakPtr. Currently not thread-safe
template<typename T>
class Ptr
{
public:

    Ptr() = default;
    Ptr(const Ptr&) = delete;
    Ptr& operator=(const Ptr&) = delete;
    Ptr(Ptr&& other) = delete;
    Ptr& operator=(Ptr&& other) = delete;
    virtual ~Ptr() = default;

    // Returns true if this Ptr is a UniquePtr, false if a WeakPtr
    virtual bool owner() const = 0;

    // Returns true if the owner is still valid. Always true for UniquePtr
    virtual bool valid() const = 0;

    // Get a naked pointer to the internally managed object
    virtual T* get() const = 0;

    // Return a weak-like ptr to safely access and check for the existence 
    // of the internally managed object
    virtual WeakPtr<T> getWeak() const = 0;

    // Get a naked pointer to the internally managed object
    T* operator->() const { return get(); }

    bool operator==(const T* other) const {return this->get()==other;}
    bool operator==(const Ptr& other) const {return this->get()==other.get();}
    bool operator!=(const T* other) const {return !((*this)==other);}
    bool operator!=(const Ptr& other) const {return !((*this)==other);}
};

//----------------------------------------------------------------------------//

// Smart ptr to an object owned by another UniquePtr. Enables validity checks
// on the lifetime of the onwer (i.e., is the owner is alive?) via the .valid()
// method. Currently not thread-safe
template<typename T>
class WeakPtr : public Ptr<T>
{
private:

    T* ptr_;
    std::weak_ptr<bool> valid_;

public:

    WeakPtr(T* p, std::shared_ptr<bool> valid) : ptr_(p), valid_(valid) {}
    WeakPtr(const WeakPtr& other) : ptr_(other.ptr_), valid_(other.valid_) {}
    WeakPtr& operator=(const WeakPtr& other)
    {
        ptr_ = other.ptr_;
        valid_ = other.valid_;
        return *this;
    };

    // WeakPtr cannot own
    bool owner() const override {return false;}

    // Returns false if the owner was destroyed
    bool valid() const override{return !valid_.expired();}
    
    // Returns ptr to the owner or nullptr if the owner was destroyed
    T* get() const override {return valid() ? ptr_ : nullptr;}

    // Return a weak-like ptr to safely access and check for the existence 
    // of the internally managed object
    WeakPtr<T> getWeak() const override {return *this;}
};

//----------------------------------------------------------------------------//

// Base class to grant any UniquePtr-owned derived class the ability to obtain
// a WeakPtr to the UniquePtr owner from within the derived class. Follows the
// spirit of std::enable_shared_from_this. Currently not thread-safe
template<typename T>
class EnableWeakFromThis 
{
friend class UniquePtr<T>; 
private:
    std::weak_ptr<bool> valid_;
protected:
    void setValid(const std::shared_ptr<bool>& valid) {valid_ = valid;}
public:
    WeakPtr<T> weakFromThis() const
    {
        return WeakPtr<T>(static_cast<T*>(this), valid_.lock());
    }
};

//----------------------------------------------------------------------------//

// Smart ptr that fundamentally acts like std::unique_ptr<T> but which allows
// taking weak ptrs to it. Currently not thread-safe
template<typename T>
class UniquePtr : public Ptr<T>
{
template<typename U>
friend class UniquePtr; // To enable move ctor with UniquePtr<D> where T is 
                        // a base of D
private:

    std::unique_ptr<T> ptr_;
    std::shared_ptr<bool> valid_;
    static constexpr bool weakFromThisEnabled_ = 
        std::is_base_of_v<EnableWeakFromThis<T>, T>;

public:

    UniquePtr() : ptr_(nullptr), valid_(nullptr) {}
    explicit UniquePtr(T* ptr) : 
        ptr_(ptr), 
        valid_(ptr ? std::make_shared<bool>(true) : nullptr) 
    {
        if constexpr (weakFromThisEnabled_) 
        {
            ptr_->setValid(valid_);
        }
    }
    UniquePtr(UniquePtr&& other) : 
        ptr_(std::move(other.ptr_)), 
        valid_(std::move(other.valid_))
    {
        if constexpr (weakFromThisEnabled_) 
        {
            if (ptr_) 
                ptr_->setValid(valid_);
        }
    }
    // To enable move ctor via UniquePtr<D> where T is a base of D
    template<typename D, typename = std::enable_if_t<
        std::is_base_of_v<T, D> && 
        !std::is_same_v<T, D>>>
    UniquePtr(UniquePtr<D>&& other) : 
        ptr_(other.ptr_.release()), 
        valid_(std::move(other.valid_))
    {
        if constexpr (weakFromThisEnabled_) 
        {
            if (ptr_) 
                ptr_->setValid(valid_);
        }
    }
    UniquePtr& operator=(UniquePtr&& other)
    {
        if (this != &other) 
        {
            reset();
            ptr_ = std::move(other.ptr_);
            valid_ = std::move(other.valid_);
            if constexpr (weakFromThisEnabled_) 
            {
                if (ptr_) 
                    ptr_->setValid(valid_);
            }
        }
        return *this;
    }
    // To enable move assignment via UniquePtr<D> where T is a base of D
    template<typename D, typename = std::enable_if_t<
        std::is_base_of_v<T, D> && 
        !std::is_same_v<T, D>>>
    UniquePtr& operator=(UniquePtr<D>&& other)
    {
        if (this->get() != static_cast<T*>(other.get()))
        {
            reset();
            ptr_.reset(other.ptr_.release());
            valid_ = std::move(other.valid_);
            if constexpr (weakFromThisEnabled_) 
            {
                if (ptr_) 
                    ptr_->setValid(valid_);
            }
        }
        return *this;
    }
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
    T& operator*() const { return *ptr_; }

    // Release ownership without destroying the object
    T* release() 
    {
        T* ptr = ptr_.release();
        valid_.reset(); // Invalidate all existing WeakPtrs to this
        return ptr;
    }

    // Destroy the managed object and reset to nullptr
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
            ptr_ = std::unique_ptr<T>(ptr);
            valid_ = ptr ? std::make_shared<bool>(true) : nullptr;
            if constexpr (weakFromThisEnabled_) 
            {
                ptr_->setValid(valid_);
            }
        }
    }

    // Reset to new pointer of a derived type
    template<typename D, typename = std::enable_if_t<
        std::is_base_of_v<T, D> && 
        !std::is_same_v<T, D>>>
    void reset(D* ptr) 
    {
        if (ptr_.get() != static_cast<T*>(ptr)) 
        {
            valid_.reset();  // Invalidate all existing WeakPtrs to this
            ptr_ = std::unique_ptr<T>(ptr);
            valid_ = ptr ? std::make_shared<bool>(true) : nullptr;
            if constexpr (weakFromThisEnabled_) 
            {
                ptr_->setValid(valid_);
            }
        }
    }

    // Return a weak-like ptr to safely access and check for the existence 
    // of the internally managed object
    WeakPtr<T> getWeak() const override {return WeakPtr<T>(ptr_.get(), valid_);}
};

//----------------------------------------------------------------------------//

// Factory method to create a new T wrapped by a UniquePtr
template<typename T, typename... Args>
static UniquePtr<T> makeUnique(Args&&... args)
{
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

//----------------------------------------------------------------------------//

// Smart ptr that enables treating any class instance like a singleton. 
// Ownership is transferred
template<class T>
class GlobalPtr
{
protected:

    static std::unique_ptr<T> ptr_;

public:

    GlobalPtr() = default;
    
    // Initialize given an instance and take ownership
    GlobalPtr(T* ptr)
    {
        if (!ptr_ && ptr)
            ptr_.reset(ptr);
    }
    
    // Initialize given an instance and take ownership. Any WeakPtrs to ptr are
    // invalidated
    GlobalPtr(UniquePtr<T>&& ptr)
    {
        if (!ptr_ && ptr != nullptr)
            ptr_.reset(ptr.release());
    }

    // Returns true if the internally managed object exists
    static bool valid() {return ptr_ != nullptr;}

    // Get a naked pointer to the internally managed object
    static T* get() {return ptr_.get();}

    // Get a naked pointer to the internally managed object
    T* operator->() const { return get(); }

    bool operator==(const T* other) const {return this->get()==other;}
    bool operator==(const GlobalPtr& other) const {return this->get()==other.get();}
    bool operator!=(const T* other) const {return !((*this)==other);}
    bool operator!=(const GlobalPtr& other) const {return !((*this)==other);}
};

template<class T>
std::unique_ptr<T> GlobalPtr<T>::ptr_ = nullptr;

}

#endif