#ifndef VSHADER_H
#define VSHADER_H

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "thirdparty/glm/glm.hpp"

namespace vir
{

class Shader
{
public:

    // Constructor specification (is the string a path to a file or a source 
    // code?)
    enum class ConstructFrom
    {
        String,
        File
    };

    // Shader variable type
    struct Variable
    {
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
            SamplerCube
        };
        uint32_t size;
        uint32_t nCmpts;
        Type type;
        typedef void* ValueType;
    };
    #define DEFINE_SHADER_VARIABLE(DT, TY, NC)  \
        struct DT : Variable                    \
        {                                       \
            DT()                                \
            {                                   \
                size=NC*sizeof(TY);             \
                nCmpts=NC;                      \
                type=Type::DT;                  \
            }                                   \
        };                                      
    DEFINE_SHADER_VARIABLE(Bool, bool, 1)
    DEFINE_SHADER_VARIABLE(UInt, uint32_t, 1)
    DEFINE_SHADER_VARIABLE(Int, int, 1)
    DEFINE_SHADER_VARIABLE(Int2, int, 2)
    DEFINE_SHADER_VARIABLE(Int3, int, 3)
    DEFINE_SHADER_VARIABLE(Int4, int, 4)
    DEFINE_SHADER_VARIABLE(Float, float, 1)
    DEFINE_SHADER_VARIABLE(Float2, float, 2)
    DEFINE_SHADER_VARIABLE(Float3, float, 3)
    DEFINE_SHADER_VARIABLE(Float4, float, 4)
    DEFINE_SHADER_VARIABLE(Mat3, float, 9)
    DEFINE_SHADER_VARIABLE(Mat4, float, 16)
    DEFINE_SHADER_VARIABLE(Sampler2D, uint32_t, 1)
    DEFINE_SHADER_VARIABLE(SamplerCube, uint32_t, 1)
    static std::unordered_map<std::string, Variable::Type> 
        valueTypeToUniformTypeMap;
    static std::unordered_map<Variable::Type, std::string>
        uniformTypeToName;
    static std::unordered_map<std::string, Variable::Type>
        uniformNameToType;
    static std::vector<std::string> uniformNames;
    static std::vector<Variable::Type> uniformTypes;

    class Uniform
    {
    private:
        bool isValueOwner_ = true;
        void* value_ = nullptr;
        bool valueIsPtr_ = false;
        // Delete copy constructors
        Uniform(const Uniform&) = delete;
        Uniform& operator=(const Uniform& other) = delete;
    public:
        std::string name = "";
        uint32_t unit = 0;
        Variable::Type type = Variable::Type::Int;
        Uniform() = default;
        ~Uniform();
        template<class ValueType>
        void setValuePtr(ValueType* value, bool isValueOwner=false)
        {
            if (value != nullptr && isValueOwner_)
                resetValue();        
            value_ = (void*)value;
            isValueOwner_ = isValueOwner;
            valueIsPtr_ = true;
        }
        template<class ValueType>
        ValueType* getValuePtr()
        {       
            if (value_ != nullptr)
                return (ValueType*)value_;
            return (ValueType*)nullptr;
        }
        template<class ValueType>
        void setValue(ValueType value)
        {
            if (value_ == nullptr)
                value_ = new ValueType(value);
            else 
                *(ValueType*)(value_) = value;
            valueIsPtr_ = false;
        }
        template<class ValueType>
        ValueType getValue()
        {
            if (value_ != nullptr)
                return *(ValueType*)(value_);
            return ValueType();
        }
        void resetValue();
        bool valueIsPtr() const {return valueIsPtr_;}
    };

protected:
    uint32_t id_;
    std::unordered_map<std::string, uint32_t> uniformMap_;
public:
    static Shader* create
    (
        std::string& vertexSource, 
        std::string& fragmentSource, 
        ConstructFrom constructFrom, 
        std::exception_ptr* exceptionPtr = nullptr
    ); 
    static Shader* create
    (
        std::string&& vertexSource, 
        std::string&& fragmentSource, 
        ConstructFrom constructFrom, 
        std::exception_ptr* exceptionPtr = nullptr
    )
    {
        std::string vs(vertexSource);
        std::string fs(fragmentSource);
        return Shader::create(vs, fs, constructFrom, exceptionPtr);
    }
    virtual ~Shader(){}
    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void setUniformBool(std::string, bool) = 0;
    virtual void setUniformUInt(std::string, uint32_t) = 0;
    virtual void setUniformInt(std::string, int) = 0;
    virtual void setUniformInt2(std::string, glm::ivec2) = 0;
    virtual void setUniformInt3(std::string, glm::ivec3) = 0;
    virtual void setUniformInt4(std::string, glm::ivec4) = 0;
    virtual void setUniformFloat(std::string, float) = 0;
    virtual void setUniformFloat2(std::string, glm::vec2) = 0;
    virtual void setUniformFloat3(std::string, glm::vec3) = 0;
    virtual void setUniformFloat4(std::string, glm::vec4) = 0;
    virtual void setUniformMat3(std::string, glm::mat3) = 0;
    virtual void setUniformMat4(std::string, glm::mat4) = 0;

    uint32_t id() const {return id_;}
};

}

#endif