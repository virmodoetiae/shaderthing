#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglshader.h"
#include "vgraphics/vcore/vbuffers.h"

namespace vir
{

std::unordered_map<std::string, Uniform::Type> 
    Shader::valueTypeToUniformTypeMap = 
{
    {typeid(bool).name(), Uniform::Type::Bool},
    {typeid(uint32_t).name(), Uniform::Type::UInt},
    {typeid(int).name(), Uniform::Type::Int},
    {typeid(glm::ivec2).name(), Uniform::Type::Int2},
    {typeid(glm::ivec3).name(), Uniform::Type::Int3},
    {typeid(glm::ivec4).name(), Uniform::Type::Int4},
    {typeid(float).name(), Uniform::Type::Float},
    {typeid(glm::vec2).name(), Uniform::Type::Float2},
    {typeid(glm::vec3).name(), Uniform::Type::Float3},
    {typeid(glm::vec4).name(), Uniform::Type::Float4},
    {typeid(glm::mat3).name(), Uniform::Type::Mat3},
    {typeid(glm::mat4).name(), Uniform::Type::Mat4}
};

std::unordered_map<Uniform::Type, std::string> 
    Shader::uniformTypeToName =
{
    {Uniform::Type::Bool, "bool"},
    {Uniform::Type::UInt, "uint"},
    {Uniform::Type::Int, "int"},
    {Uniform::Type::Int2, "ivec2"},
    {Uniform::Type::Int3, "ivec3"},
    {Uniform::Type::Int4, "ivec4"},
    {Uniform::Type::Float, "float"},
    {Uniform::Type::Float2, "vec2"},
    {Uniform::Type::Float3, "vec3"},
    {Uniform::Type::Float4, "vec4"},
    {Uniform::Type::Mat3, "mat3"},
    {Uniform::Type::Mat4, "mat4"},
    {Uniform::Type::Sampler2D, "sampler2D"},
    {Uniform::Type::Sampler3D, "sampler3D"},
    {Uniform::Type::SamplerCube, "samplerCube"},
    {Uniform::Type::Image2D, "image2D"},
    {Uniform::Type::Image3D, "image3D"},
    {Uniform::Type::ImageCube, "imageCube"}
};

std::unordered_map<std::string, Uniform::Type> 
    Shader::uniformNameToType =
{
    {"bool", Uniform::Type::Bool},
    {"uint", Uniform::Type::UInt},
    {"int", Uniform::Type::Int},
    {"ivec2", Uniform::Type::Int2},
    {"ivec3", Uniform::Type::Int3},
    {"ivec4", Uniform::Type::Int4},
    {"float", Uniform::Type::Float},
    {"vec2", Uniform::Type::Float2},
    {"vec3", Uniform::Type::Float3},
    {"vec4", Uniform::Type::Float4},
    {"mat3", Uniform::Type::Mat3},
    {"mat4", Uniform::Type::Mat4},
    {"sampler2D", Uniform::Type::Sampler2D},
    {"sampler3D", Uniform::Type::Sampler3D},
    {"samplerCube", Uniform::Type::SamplerCube},
    {"image2D", Uniform::Type::Image2D},
    {"image3D", Uniform::Type::Image3D},
    {"imageCube", Uniform::Type::ImageCube}
};

std::vector<Uniform::Type> Shader::uniformTypes = 
{
    Uniform::Type::Bool,
    Uniform::Type::UInt,
    Uniform::Type::Int,
    Uniform::Type::Int2,
    Uniform::Type::Int3,
    Uniform::Type::Int4,
    Uniform::Type::Float,
    Uniform::Type::Float2,
    Uniform::Type::Float3,
    Uniform::Type::Float4,
    Uniform::Type::Mat3,
    Uniform::Type::Mat4,
    Uniform::Type::Sampler2D,
    Uniform::Type::Sampler3D,
    Uniform::Type::SamplerCube,
    Uniform::Type::Image2D,
    Uniform::Type::Image3D,
    Uniform::Type::ImageCube
};

std::vector<std::string> Shader::uniformNames = 
{
    "bool",
    "uint",
    "int",
    "ivec2",
    "ivec3",
    "ivec4",
    "float",
    "vec2",
    "vec3",
    "vec4",
    "mat3",
    "mat4",
    "sampler2D",
    "sampler3D",
    "samplerCube",
    "image2D",
    "image3D",
    "imageCube"
};

std::unordered_map<std::string, bool> 
    Shader::currentContextExtensionsStatusMap_ = {};

Uniform::~Uniform()
{
    deleteValue();
}

void Uniform::deleteValue(bool deleteCache)
{
    #define CASE_DELETE(type, cppType)                  \
        case Uniform::Type::type :                      \
            if (isArray)                                \
                delete[] static_cast<cppType*>(value);  \
            else                                        \
                delete static_cast<cppType*>(value);    \
            break;
    
    auto _delete = [](void*& value, Type type, bool isArray)
    {
        if (value == nullptr)
            return;
        switch(type)
        {
            CASE_DELETE(Bool, bool)
            CASE_DELETE(UInt, uint32_t)
            CASE_DELETE(Int, int)
            CASE_DELETE(Int2, glm::ivec2)
            CASE_DELETE(Int3, glm::ivec3)
            CASE_DELETE(Int4, glm::ivec4)
            CASE_DELETE(Float, float)
            CASE_DELETE(Float2, glm::vec2)
            CASE_DELETE(Float3, glm::vec3)
            CASE_DELETE(Float4, glm::vec4)
            CASE_DELETE(Mat3, glm::mat3)
            CASE_DELETE(Mat4, glm::mat4)
            case Uniform::Type::Sampler2D :
            case Uniform::Type::Image2D :
                delete static_cast<TextureBuffer2D*>(value);
                break;
            case Uniform::Type::Sampler3D :
            case Uniform::Type::Image3D :
                delete static_cast<TextureBuffer3D*>(value);
                break;
            case Uniform::Type::SamplerCube :
            case Uniform::Type::ImageCube :
                delete static_cast<CubeMapBuffer*>(value);
                break;
        }
        value = nullptr;
    };

    if (deleteCache)
        _delete(cache_, type_, false);
    if (isValueOwner_)
        _delete(value_, type_, valueArraySize_ > 1);
    value_ = nullptr;
}

void Uniform::setType
(
    Type type, 
    uint32_t valueArraySize,
    bool doNotReinitializeIfImageOrSampler
)
{
    if (value_ != nullptr && type == type_ && valueArraySize == valueArraySize_)
        return;
    if 
    (
        doNotReinitializeIfImageOrSampler && 
        (
            (type == Type::Image2D      && type_ == Type::Sampler2D)    || 
            (type == Type::Sampler2D    && type_ == Type::Image2D)      ||
            (type == Type::Image3D      && type_ == Type::Sampler3D)    || 
            (type == Type::Sampler3D    && type_ == Type::Image3D)      ||
            (type == Type::ImageCube    && type_ == Type::SamplerCube)  || 
            (type == Type::SamplerCube  && type_ == Type::ImageCube)
        )
    )
    {
        type_ = type;
        return;
    }

    if (value_ != nullptr)
        deleteValue(type != type_);
    isValueOwner_ = true;
    type_ = type;
    valueArraySize_ = valueArraySize;

    #define CASE_INITIALIZE(type, cppType)              \
        case Uniform::Type::type :                      \
            if (valueArraySize > 1)                     \
                value_ = new cppType[valueArraySize](); \
            else                                        \
                value_ = new cppType();                 \
            break;

    switch(type)
    {
        CASE_INITIALIZE(Bool, bool)
        CASE_INITIALIZE(UInt, uint32_t)
        CASE_INITIALIZE(Int, int)
        CASE_INITIALIZE(Int2, glm::ivec2)
        CASE_INITIALIZE(Int3, glm::ivec3)
        CASE_INITIALIZE(Int4, glm::ivec4)
        CASE_INITIALIZE(Float, float)
        CASE_INITIALIZE(Float2, glm::vec2)
        CASE_INITIALIZE(Float3, glm::vec3)
        CASE_INITIALIZE(Float4, glm::vec4)
        CASE_INITIALIZE(Mat3, glm::mat3)
        CASE_INITIALIZE(Mat4, glm::mat4)
        case Uniform::Type::Sampler2D :
        case Uniform::Type::Image2D :
        case Uniform::Type::Sampler3D :
        case Uniform::Type::Image3D :
        case Uniform::Type::SamplerCube :
        case Uniform::Type::ImageCube :
            break;
    }
}

void Uniform::setType
(
    Type type, 
    bool doNotReinitializeIfImageOrSampler
)
{
    setType(type, doNotReinitializeIfImageOrSampler, valueArraySize_);
}

UniquePtr<Shader> Shader::create
(
    const std::string& vs, 
    const std::string& fs, 
    ConstructFrom cf
)
{
    if (!GlobalPtr<Window>::valid())
        return UniquePtr<Shader>();
    switch(Window::instance()->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return makeUnique<OpenGLShader>(vs, fs, cf);
    }
    return UniquePtr<Shader>();
}

//----------------------------------------------------------------------------//

std::string Shader::currentContextShadingLanguageDirectives()
{
    if (!GlobalPtr<Window>::valid())
        return nullptr;
    switch (Window::instance()->context()->type())
    {
    case GraphicsContext::Type::OpenGL :
        return OpenGLShader::currentContextShadingLanguageDirectives();
    default:
        return "";
    }
}

bool Shader::setExtensionStatusInCurrentContextShadingLanguageDirectives
(
    const std::string& extensionName,
    bool status
)
{
    if (!GlobalPtr<Window>::valid())
        return false;
    switch (Window::instance()->context()->type())
    {
    case GraphicsContext::Type::OpenGL :
        return OpenGLShader::
               setExtensionStatusInCurrentContextShadingLanguageDirectives
        (
            extensionName,
            status
        );
    default:
        return false;
    }
}

bool Shader::isExtensionInCurrentContextShadingLanguageDirectives
(
    const std::string& extensionName
)
{
    if 
    (
        currentContextExtensionsStatusMap_.find
        (
            extensionName
        ) != currentContextExtensionsStatusMap_.end()
    )
        return currentContextExtensionsStatusMap_.at(extensionName);
    return false;
}

std::vector<std::string> 
Shader::extensionsInCurrentContextShadingLanguageDirectives()
{
    std::vector<std::string> extensions(0);
    for (const auto& item : currentContextExtensionsStatusMap_)
    {
        if (item.second)
            extensions.push_back(item.first);
    }
    return extensions;
}

}