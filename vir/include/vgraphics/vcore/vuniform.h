#ifndef VUNIFORM_H
#define VUNIFORM_H

#include <string>
#include <thirdparty/glm/glm.hpp>

#include "vpointers.h"

namespace vir
{

class DynamicUniformBuffer;

class Uniform : public EnableWeakFromThis<Uniform>
{

friend DynamicUniformBuffer; // Because it needs to modify clientBuffers_

public:

    enum class Type
    {
        Bool,
        UInt,
        Int,       Int2,      Int3,   Int4,
        Float,     Float2,    Float3, Float4,
        Mat3,      Mat4,
        Sampler2D, Sampler3D, SamplerCube,
        Image2D,   Image3D,   ImageCube
    };

private:

    bool     isValueOwner_   = true;
    void*    value_          = nullptr;
    uint32_t valueArraySize_ = 1;
    Type     type_           = Type::Int;
    // The cache serves as an additional back-up storage value that
    // can be read/set via the corresponding methods
    void*    cache_          = nullptr;
    // List of DynamicUniformBuffers to which this uniform was added. A uniform
    // can be simultaneously registered in multiple different buffers. 
    // Automatically managed by Uniform::~Uniform(), 
    // DynamicUniformBuffer::addUniform(), DynamicUniformBuffer::removeUniform()
    std::vector<WeakPtr<DynamicUniformBuffer>> clientBuffers_  = {};
    
    Uniform(const Uniform&)                  = delete;
    Uniform& operator=(const Uniform& other) = delete;
    
    void setType
    (
        Type type, 
        uint32_t valueArraySize,
        bool doNotReinitializeIfImageOrSampler
    );

protected:
    
    // Protected ctor: objects are meant to be initialized via the ::create()
    // method
    Uniform();
    virtual void deleteValue(bool deleteCache=true);

public:
    
    // Uniform name
    std::string name = "";

    // Texture unit to which the sampler is bound if the native type consists
    // of a SamplerXX type resource
    uint32_t unit = 0;
    
    static UniquePtr<Uniform> create();
    virtual ~Uniform();
    
    // (Re)set the uniform type. If doNotReinitializeIfImageOrSampler = true,
    // the native value will not be reset if the uniform type consists of
    // any of the ImageXX or SamplerXX uniform types. This allows e.g., 
    // switching the uniform type while mainting the same e.g., texture as 
    // uniform value
    void setType
    (
        Type type, 
        bool doNotReinitializeIfImageOrSampler = false 
    );

    // Submits this uniform to all dynamic uniform buffers with which
    // it was registered
    void submitToAllClientBuffers();

    // Marks this uniform for submission to all dynamic uniform buffers
    // witch which it was registered. This does not actually submit the
    // uniform, which will have to be done on a per-buffer-basis via
    // DynamicUniformBuffer::submitUniforms()
    void markForSubmissionToAllClientBuffers();

    // Unregisters this uniform from all dynamic uniform buffers with
    // which it was registered
    void removeFromAllClientBuffers();

    // Returns a naked pointer to the native uniform value
    const void* getNativeValue() const {return value_;}

    // If the native value is an array, returns its size, else returns 1.
    uint32_t valueArraySize() const {return valueArraySize_;}

    // Returns true if the native value is an array
    bool isValueArray() const {return valueArraySize_ > 1;}

    // Returns the uniform type
    Type type() const {return type_;}
    
    // Sets the native value of this uniform to a pointer to another variable
    // of a compatible type. Ownership can transferred by setting 
    // isValueOwner = true. Any existing native value is deleted if this
    // uniform owned it (isValueOwner_ = true)
    template<class ValueType>
    void setValuePtr
    (
        ValueType* value, 
        Type type, 
        uint32_t valueArraySize=1, 
        bool isValueOwner=false
    )
    {
        deleteValue(type != type_);
        value_ = (void*)value;
        valueArraySize_ = valueArraySize;
        type_ = type;
        isValueOwner_ = isValueOwner;
    }
    
    // Returns the naked native value pointer (regardless of ownership)
    template<class ValueType>
    ValueType* getValuePtr()
    {       
        if (value_ != nullptr)
            return (ValueType*)value_;
        return (ValueType*)nullptr;
    }
    
    // Returns the naked native value pointer (regardless of ownership) as a 
    // const ptr
    template<class ValueType>
    const ValueType* getConstValuePtr() const
    {       
        if (value_ != nullptr)
            return (const ValueType*)value_;
        return (const ValueType*)nullptr;
    }
    
    // Sets the value of the native value to the provided value. If the new
    // value type differs from the current uniform type, the uniform is
    // reset (and the previous native value is deleted if owned) prior to
    // setting the new value
    template<class ValueType>
    void setValue(ValueType value, Type type, uint32_t valueArraySize=1)
    {
        setType(type, valueArraySize, false); // Also does reinitialization 
                                              // if type != type_
        *(ValueType*)(value_) = value;
        isValueOwner_ = true;
    }
    
    // Returns the native value if the native value is initialized, else
    // returns a default-initialized value of the current uniform type
    template<class ValueType>
    ValueType getValue() const
    {
        if (value_ != nullptr)
            return *(ValueType*)(value_);
        return ValueType();
    }
    
    // 
    template<class ValueType>
    void setCache(ValueType value)
    {
        if (cache_ == nullptr)
            cache_ = (void*) new ValueType(value);
        else 
            *(ValueType*)(cache_) = value;
    }
    
    //
    template<class ValueType>
    ValueType getCache()
    {
        if (cache_ != nullptr)
            return *(ValueType*)(cache_);
        return ValueType();
    }
};

}

#endif