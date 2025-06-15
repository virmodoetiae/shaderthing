#ifndef VUNIFORM_H
#define VUNIFORM_H

#include <string>
#include <thirdparty/glm/glm.hpp>

namespace vir
{

class Uniform
{
public:
    enum class Type
    {
        Bool,
        UInt,
        Int,
        Int2,
        Int3,
        Int4,
        Float,
        Float2,
        Float3,
        Float4,
        Mat3,
        Mat4,
        Sampler2D,
        Sampler3D,
        SamplerCube,
        Image2D,
        Image3D,
        ImageCube
    };
private:
    bool           isValueOwner_ = true;
    void*          value_        = nullptr;
    uint32_t       valueArraySize_ = 1;
    Type           type_         = Type::Int;
    // The cache serves as an additional back-up storage value that
    // can be read/set via the corresponding methods
    void*          cache_        = nullptr;
    Uniform(const Uniform&) = delete;
    Uniform& operator=(const Uniform& other) = delete;
    void setType
    (
        Type type, 
        uint32_t valueArraySize,
        bool doNotReinitializeIfImageOrSampler
    );
protected:
    virtual void deleteValue(bool deleteCache=true);
public:
    std::string    name = "";
    uint32_t       unit = 0;
    Uniform() = default;
    virtual ~Uniform();
    void setType
    (
        Type type, 
        bool doNotReinitializeIfImageOrSampler = false 
    );
    const void* getNativeValue() const {return value_;}
    uint32_t valueArraySize() const {return valueArraySize_;}
    bool isValueArray() const {return valueArraySize_ > 1;}
    Type type() const {return type_;}
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
    template<class ValueType>
    ValueType* getValuePtr()
    {       
        if (value_ != nullptr)
            return (ValueType*)value_;
        return (ValueType*)nullptr;
    }
    template<class ValueType>
    const ValueType* getConstValuePtr() const
    {       
        if (value_ != nullptr)
            return (const ValueType*)value_;
        return (const ValueType*)nullptr;
    }
    template<class ValueType>
    void setValue(ValueType value, Type type, uint32_t valueArraySize=1)
    {
        setType(type, valueArraySize, false); // Also does reinitialization 
                                                // if type != type_
        *(ValueType*)(value_) = value;
        isValueOwner_ = true;
    }
    template<class ValueType>
    ValueType getValue() const
    {
        if (value_ != nullptr)
            return *(ValueType*)(value_);
        return ValueType();
    }
    template<class ValueType>
    void setCache(ValueType value)
    {
        if (cache_ == nullptr)
            cache_ = (void*) new ValueType(value);
        else 
            *(ValueType*)(cache_) = value;
    }
    template<class ValueType>
    ValueType getCache()
    {
        if (cache_ != nullptr)
            return *(ValueType*)(cache_);
        return ValueType();
    }
};

template<typename T>
struct UniformTypeTraits;
template<> struct UniformTypeTraits<bool>       {static constexpr Uniform::Type value=Uniform::Type::Bool;};
template<> struct UniformTypeTraits<uint32_t>   {static constexpr Uniform::Type value=Uniform::Type::UInt;};
template<> struct UniformTypeTraits<int>        {static constexpr Uniform::Type value=Uniform::Type::Int;};
template<> struct UniformTypeTraits<glm::ivec2> {static constexpr Uniform::Type value=Uniform::Type::Int2;};
template<> struct UniformTypeTraits<glm::ivec3> {static constexpr Uniform::Type value=Uniform::Type::Int3;};
template<> struct UniformTypeTraits<glm::ivec4> {static constexpr Uniform::Type value=Uniform::Type::Int4;};
template<> struct UniformTypeTraits<float>      {static constexpr Uniform::Type value=Uniform::Type::Float;};
template<> struct UniformTypeTraits<glm::vec2>  {static constexpr Uniform::Type value=Uniform::Type::Float2;};
template<> struct UniformTypeTraits<glm::vec3>  {static constexpr Uniform::Type value=Uniform::Type::Float3;};
template<> struct UniformTypeTraits<glm::vec4>  {static constexpr Uniform::Type value=Uniform::Type::Float4;};
template<> struct UniformTypeTraits<glm::mat3>  {static constexpr Uniform::Type value=Uniform::Type::Mat3;};
template<> struct UniformTypeTraits<glm::mat4>  {static constexpr Uniform::Type value=Uniform::Type::Mat4;};

template<typename ValueType>
class TypedUniform : protected Uniform
{
public:
    explicit TypedUniform(uint32_t valueArraySize = 1)
    {
        setType(UniformTypeTraits<ValueType>::value, valueArraySize);
    }
    uint32_t valueArraySize() const {return valueArraySize_;}
    bool isValueArray() const {return valueArraySize_ > 1;}
    void setValuePtr
    (
        ValueType* value, 
        uint32_t valueArraySize=1, 
        bool isValueOwner=false
    )
    {
        deleteValue(false);
        value_ = (void*)value;
        valueArraySize_ = valueArraySize;
        isValueOwner_ = isValueOwner;
    }
    ValueType* getValuePtr()
    {       
        if (value_ != nullptr)
            return (ValueType*)value_;
        return (ValueType*)nullptr;
    }
    const ValueType* getConstValuePtr() const
    {       
        if (value_ != nullptr)
            return (const ValueType*)value_;
        return (const ValueType*)nullptr;
    }
    void setValue(ValueType value, uint32_t valueArraySize=1)
    {
        setType(type_, valueArraySize, false);
        *(ValueType*)(value_) = value;
        isValueOwner_ = true;
    }
    ValueType getValue() const
    {
        if (value_ != nullptr)
            return *(ValueType*)(value_);
        return ValueType();
    }
    void setCache(ValueType value)
    {
        if (cache_ == nullptr)
            cache_ = (void*) new ValueType(value);
        else 
            *(ValueType*)(cache_) = value;
    }
    ValueType getCache()
    {
        if (cache_ != nullptr)
            return *(ValueType*)(cache_);
        return ValueType();
    }
};
typedef TypedUniform<bool>       UniformBool;
typedef TypedUniform<uint32_t>   UniformUInt;
typedef TypedUniform<int>        UniformInt;
typedef TypedUniform<glm::ivec2> UniformInt2;
typedef TypedUniform<glm::ivec3> UniformInt3;
typedef TypedUniform<glm::ivec4> UniformInt4;
typedef TypedUniform<float>      UniformFloat;
typedef TypedUniform<glm::vec2>  UniformFloat2;
typedef TypedUniform<glm::vec3>  UniformFloat3;
typedef TypedUniform<glm::vec4>  UniformFloat4;
typedef TypedUniform<glm::mat3>  UniformMat3;
typedef TypedUniform<glm::mat4>  UniformMat4;

}

#endif