#ifndef VSHADER_H
#define VSHADER_H

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "vir/include/vmacros.h"
#include "vir/include/vpointers.h"
#include "thirdparty/glm/glm.hpp"

namespace vir
{

class Shader
{
public:

    enum class ConstructFrom
    {
        // The passed string is the source code itself
        SourceCode,
        // The passed string is a filepath to the source code
        SourceFile 
    };

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
            setType(type, valueArraySize, false); // Also does reinitialization if type != type_
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

    struct CompilationErrors
    {
        std::map<int, std::string> vertexErrors   = {};
        std::map<int, std::string> fragmentErrors = {};
        unsigned int size() const 
        {
            return vertexErrors.size() + fragmentErrors.size();
        }
    };

    static std::unordered_map<std::string, Uniform::Type> 
        valueTypeToUniformTypeMap;
    static std::unordered_map<Uniform::Type, std::string>
        uniformTypeToName;
    static std::unordered_map<std::string, Uniform::Type>
        uniformNameToType;
    static std::vector<std::string> uniformNames;
    static std::vector<Uniform::Type> uniformTypes;

protected:
    uint32_t id_;
    std::unordered_map<std::string, uint32_t> uniformMap_;
    CompilationErrors compilationErrors_;
    // Map of all extensions supported by the graphics context to their 
    // respective status in the shading language, i.e., is said extension
    // included in the shading language directives returned by 
    // shadingLanguageDirectives()? Only useful for OpenGL (as far as I know)
    static std::unordered_map<std::string, bool> 
        currentContextExtensionsStatusMap_;
    Shader() = default;
    DELETE_COPY_MOVE(Shader)
public:
    static UniquePtr<Shader> create
    (
        const std::string& vertexSource, 
        const std::string& fragmentSource, 
        ConstructFrom constructFrom
    ); 
    virtual ~Shader() = default;
    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void setUniformBool  (const std::string&, bool) = 0;
    virtual void setUniformUInt  (const std::string&, uint32_t) = 0;
    virtual void setUniformInt   (const std::string&, int) = 0;
    virtual void setUniformInt2  (const std::string&, glm::ivec2) = 0;
    virtual void setUniformInt3  (const std::string&, glm::ivec3) = 0;
    virtual void setUniformInt4  (const std::string&, glm::ivec4) = 0;
    virtual void setUniformFloat (const std::string&, float) = 0;
    virtual void setUniformFloat2(const std::string&, glm::vec2) = 0;
    virtual void setUniformFloat3(const std::string&, glm::vec3) = 0;
    virtual void setUniformFloat4(const std::string&, glm::vec4) = 0;
    virtual void setUniformMat3  (const std::string&, glm::mat3) = 0;
    virtual void setUniformMat4  (const std::string&, glm::mat4) = 0;

    // Locates and binds a named uniform block in this shader to the provided
    // bindingPoint
    virtual void bindUniformBlock
    (
        const std::string& blockName,  
        uint32_t bindingPoint
    ) = 0;

    // Locates and binds a named shader storage block in this shader to the
    // provided bindingPoint
    virtual void bindShaderStorageBlock
    (
        const std::string& blockName,  
        uint32_t bindingPoint
    ) = 0;

    uint32_t id() const {return id_;}
    const CompilationErrors& compilationErrors() const 
    {
        return compilationErrors_;
    }
    bool valid() const {return compilationErrors_.size() == 0;}

    static std::string currentContextShadingLanguageDirectives();
    static bool setExtensionStatusInCurrentContextShadingLanguageDirectives
    (
        const std::string& extensionName,
        bool status
    );
    static bool isExtensionInCurrentContextShadingLanguageDirectives
    (
        const std::string& extensionName
    );
    static std::vector<std::string> 
        extensionsInCurrentContextShadingLanguageDirectives();
};

}

#endif