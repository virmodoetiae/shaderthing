#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglshader.h"
#include "vgraphics/vcore/vbuffers.h"

namespace vir
{

std::unordered_map<std::string, Shader::Uniform::Type> 
    Shader::valueTypeToUniformTypeMap = 
{
    {typeid(bool).name(), Shader::Uniform::Type::Bool},
    {typeid(uint32_t).name(), Shader::Uniform::Type::UInt},
    {typeid(int).name(), Shader::Uniform::Type::Int},
    {typeid(glm::ivec2).name(), Shader::Uniform::Type::Int2},
    {typeid(glm::ivec3).name(), Shader::Uniform::Type::Int3},
    {typeid(glm::ivec4).name(), Shader::Uniform::Type::Int4},
    {typeid(float).name(), Shader::Uniform::Type::Float},
    {typeid(glm::vec2).name(), Shader::Uniform::Type::Float2},
    {typeid(glm::vec3).name(), Shader::Uniform::Type::Float3},
    {typeid(glm::vec4).name(), Shader::Uniform::Type::Float4},
    {typeid(glm::mat3).name(), Shader::Uniform::Type::Mat3},
    {typeid(glm::mat4).name(), Shader::Uniform::Type::Mat4}
};

std::unordered_map<Shader::Uniform::Type, std::string> 
    Shader::uniformTypeToName =
{
    {Shader::Uniform::Type::Bool, "bool"},
    {Shader::Uniform::Type::UInt, "uint"},
    {Shader::Uniform::Type::Int, "int"},
    {Shader::Uniform::Type::Int2, "ivec2"},
    {Shader::Uniform::Type::Int3, "ivec3"},
    {Shader::Uniform::Type::Int4, "ivec4"},
    {Shader::Uniform::Type::Float, "float"},
    {Shader::Uniform::Type::Float2, "vec2"},
    {Shader::Uniform::Type::Float3, "vec3"},
    {Shader::Uniform::Type::Float4, "vec4"},
    {Shader::Uniform::Type::Mat3, "mat3"},
    {Shader::Uniform::Type::Mat4, "mat4"},
    {Shader::Uniform::Type::Sampler2D, "sampler2D"},
    {Shader::Uniform::Type::Sampler3D, "sampler3D"},
    {Shader::Uniform::Type::SamplerCube, "samplerCube"},
    {Shader::Uniform::Type::Image2D, "image2D"},
    {Shader::Uniform::Type::Image3D, "image3D"},
    {Shader::Uniform::Type::ImageCube, "imageCube"}
};

std::unordered_map<std::string, Shader::Uniform::Type> 
    Shader::uniformNameToType =
{
    {"bool", Shader::Uniform::Type::Bool},
    {"uint", Shader::Uniform::Type::UInt},
    {"int", Shader::Uniform::Type::Int},
    {"ivec2", Shader::Uniform::Type::Int2},
    {"ivec3", Shader::Uniform::Type::Int3},
    {"ivec4", Shader::Uniform::Type::Int4},
    {"float", Shader::Uniform::Type::Float},
    {"vec2", Shader::Uniform::Type::Float2},
    {"vec3", Shader::Uniform::Type::Float3},
    {"vec4", Shader::Uniform::Type::Float4},
    {"mat3", Shader::Uniform::Type::Mat3},
    {"mat4", Shader::Uniform::Type::Mat4},
    {"sampler2D", Shader::Uniform::Type::Sampler2D},
    {"sampler3D", Shader::Uniform::Type::Sampler3D},
    {"samplerCube", Shader::Uniform::Type::SamplerCube},
    {"image2D", Shader::Uniform::Type::Image2D},
    {"image3D", Shader::Uniform::Type::Image3D},
    {"imageCube", Shader::Uniform::Type::ImageCube}
};

std::vector<Shader::Uniform::Type> Shader::uniformTypes = 
{
    Shader::Uniform::Type::Bool,
    Shader::Uniform::Type::UInt,
    Shader::Uniform::Type::Int,
    Shader::Uniform::Type::Int2,
    Shader::Uniform::Type::Int3,
    Shader::Uniform::Type::Int4,
    Shader::Uniform::Type::Float,
    Shader::Uniform::Type::Float2,
    Shader::Uniform::Type::Float3,
    Shader::Uniform::Type::Float4,
    Shader::Uniform::Type::Mat3,
    Shader::Uniform::Type::Mat4,
    Shader::Uniform::Type::Sampler2D,
    Shader::Uniform::Type::Sampler3D,
    Shader::Uniform::Type::SamplerCube,
    Shader::Uniform::Type::Image2D,
    Shader::Uniform::Type::Image3D,
    Shader::Uniform::Type::ImageCube
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

Shader::Uniform::~Uniform()
{
    deleteValue();
}

void Shader::Uniform::deleteValue(bool deleteCache)
{
    auto _delete = [](void*& value, Type type)
    {
        if (value == nullptr)
            return;
        switch(type)
        {
            case Uniform::Type::Bool :
                delete static_cast<bool*>(value); 
                break;
            case Uniform::Type::UInt :
                delete static_cast<uint32_t*>(value);
                break;
            case Uniform::Type::Int :
                delete static_cast<int32_t*>(value);
                break;
            case Uniform::Type::Int2 :
                delete static_cast<glm::ivec2*>(value);
                break;
            case Uniform::Type::Int3 :
                delete static_cast<glm::ivec3*>(value);
                break;
            case Uniform::Type::Int4 :
                delete static_cast<glm::ivec4*>(value);
                break;
            case Uniform::Type::Float :
                delete static_cast<float*>(value);
                break;
            case Uniform::Type::Float2 :
                delete static_cast<glm::vec2*>(value);
                break;
            case Uniform::Type::Float3 :
                delete static_cast<glm::vec3*>(value);
                break;
            case Uniform::Type::Float4 :
                delete static_cast<glm::vec4*>(value);
                break;
            case Uniform::Type::Mat3 :
                delete static_cast<glm::mat3*>(value);
                break;
            case Uniform::Type::Mat4 :
                delete static_cast<glm::mat4*>(value);
                break;
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
        _delete(cache_, type_);
    if (isValueOwner_)
        _delete(value_, type_);
    value_ = nullptr;
}

void Shader::Uniform::setType
(
    Type type, 
    bool doNotReinitializeIfImageOrSampler
)
{
    if (value_ != nullptr && type == type_)
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
    switch(type)
    {
        case Uniform::Type::Bool :
            value_ = new bool();
            break;
        case Uniform::Type::UInt :
            value_ = new uint32_t();
            break;
        case Uniform::Type::Int :
            value_ = new int32_t();
            break;
        case Uniform::Type::Int2 :
            value_ = new glm::ivec2();
            break;
        case Uniform::Type::Int3 :
            value_ = new glm::ivec3();
            break;
        case Uniform::Type::Int4 :
            value_ = new glm::ivec4();
            break;
        case Uniform::Type::Float :
            value_ = new float();
            break;
        case Uniform::Type::Float2 :
            value_ = new glm::vec2();
            break;
        case Uniform::Type::Float3 :
            value_ = new glm::vec3();
            break;
        case Uniform::Type::Float4 :
            value_ = new glm::vec4();
            break;
        case Uniform::Type::Mat3 :
            value_ = new glm::mat3();
            break;
        case Uniform::Type::Mat4 :
            value_ = new glm::mat4();
            break;
        case Uniform::Type::Sampler2D :
        case Uniform::Type::Image2D :
        case Uniform::Type::Sampler3D :
        case Uniform::Type::Image3D :
        case Uniform::Type::SamplerCube :
        case Uniform::Type::ImageCube :
            break;
    }
}

Shader* Shader::create
(
    const std::string& vs, 
    const std::string& fs, 
    ConstructFrom cf
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLShader(vs, fs, cf);
    }
    return nullptr;
}

//----------------------------------------------------------------------------//

std::string Shader::currentContextShadingLanguageDirectives()
{
    static auto* context = vir::GlobalPtr<vir::Window>::instance()->context();
    switch (context->type())
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
    static auto* context = vir::GlobalPtr<vir::Window>::instance()->context();
    switch (context->type())
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