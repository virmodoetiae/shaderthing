#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglshader.h"
#include "vgraphics/vcore/vbuffers.h"

namespace vir
{

std::unordered_map<std::string, Shader::Variable::Type> 
    Shader::valueTypeToUniformTypeMap = 
{
    {typeid(bool).name(), Shader::Variable::Type::Bool},
    {typeid(uint32_t).name(), Shader::Variable::Type::UInt},
    {typeid(int).name(), Shader::Variable::Type::Int},
    {typeid(glm::ivec2).name(), Shader::Variable::Type::Int2},
    {typeid(glm::ivec3).name(), Shader::Variable::Type::Int3},
    {typeid(glm::ivec4).name(), Shader::Variable::Type::Int4},
    {typeid(float).name(), Shader::Variable::Type::Float},
    {typeid(glm::vec2).name(), Shader::Variable::Type::Float2},
    {typeid(glm::vec3).name(), Shader::Variable::Type::Float3},
    {typeid(glm::vec4).name(), Shader::Variable::Type::Float4},
    {typeid(glm::mat3).name(), Shader::Variable::Type::Mat3},
    {typeid(glm::mat4).name(), Shader::Variable::Type::Mat4}
};

std::unordered_map<Shader::Variable::Type, std::string> 
    Shader::uniformTypeToName =
{
    {Shader::Variable::Type::Bool, "bool"},
    {Shader::Variable::Type::UInt, "uint"},
    {Shader::Variable::Type::Int, "int"},
    {Shader::Variable::Type::Int2, "ivec2"},
    {Shader::Variable::Type::Int3, "ivec3"},
    {Shader::Variable::Type::Int4, "ivec4"},
    {Shader::Variable::Type::Float, "float"},
    {Shader::Variable::Type::Float2, "vec2"},
    {Shader::Variable::Type::Float3, "vec3"},
    {Shader::Variable::Type::Float4, "vec4"},
    {Shader::Variable::Type::Mat3, "mat3"},
    {Shader::Variable::Type::Mat4, "mat4"},
    {Shader::Variable::Type::Sampler2D, "texture2D"},
    {Shader::Variable::Type::SamplerCube, "cubemap"}
};

std::unordered_map<std::string, Shader::Variable::Type> 
    Shader::uniformNameToType =
{
    {"bool", Shader::Variable::Type::Bool},
    {"uint", Shader::Variable::Type::UInt},
    {"int", Shader::Variable::Type::Int},
    {"ivec2", Shader::Variable::Type::Int2},
    {"ivec3", Shader::Variable::Type::Int3},
    {"ivec4", Shader::Variable::Type::Int4},
    {"float", Shader::Variable::Type::Float},
    {"vec2", Shader::Variable::Type::Float2},
    {"vec3", Shader::Variable::Type::Float3},
    {"vec4", Shader::Variable::Type::Float4},
    {"mat3", Shader::Variable::Type::Mat3},
    {"mat4", Shader::Variable::Type::Mat4},
    {"texture2D", Shader::Variable::Type::Sampler2D},
    {"cubemap", Shader::Variable::Type::SamplerCube}
};

std::vector<Shader::Variable::Type> Shader::uniformTypes = 
{
    Shader::Variable::Type::Bool,
    Shader::Variable::Type::UInt,
    Shader::Variable::Type::Int,
    Shader::Variable::Type::Int2,
    Shader::Variable::Type::Int3,
    Shader::Variable::Type::Int4,
    Shader::Variable::Type::Float,
    Shader::Variable::Type::Float2,
    Shader::Variable::Type::Float3,
    Shader::Variable::Type::Float4,
    Shader::Variable::Type::Mat3,
    Shader::Variable::Type::Mat4,
    Shader::Variable::Type::Sampler2D,
    Shader::Variable::Type::SamplerCube
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
    "texture2D",
    "cubemap"
};

std::unordered_map<std::string, bool> 
    Shader::currentContextExtensionsStatusMap_ = {};

Shader::Uniform::~Uniform()
{
    resetValue();
}

void Shader::Uniform::resetValue()
{
    if (!isValueOwner_)
    {
        value_ = nullptr;
        return;
    }
    if(value_ == nullptr)
        return;
    switch(type)
    {
        case Variable::Type::Bool :
            delete static_cast<bool*>(value_);
            break;
        case Variable::Type::UInt :
            delete static_cast<uint32_t*>(value_);
            break;
        case Variable::Type::Int :
            delete static_cast<int*>(value_);
            break;
        case Variable::Type::Int2 :
            delete static_cast<glm::ivec2*>(value_);
            break;
        case Variable::Type::Int3 :
            delete static_cast<glm::ivec3*>(value_);
            break;
        case Variable::Type::Int4 :
            delete static_cast<glm::ivec4*>(value_);
            break;
        case Variable::Type::Float :
            delete static_cast<float*>(value_);
            break;
        case Variable::Type::Float2 :
            delete static_cast<glm::vec2*>(value_);
            break;
        case Variable::Type::Float3 :
            delete static_cast<glm::vec3*>(value_);
            break;
        case Variable::Type::Float4 :
            delete static_cast<glm::vec4*>(value_);
            break;
        case Variable::Type::Mat3 :
            delete static_cast<glm::mat3*>(value_);
            break;
        case Variable::Type::Mat4 :
            delete static_cast<glm::mat4*>(value_);
            break;
        case Variable::Type::Sampler2D :
            delete static_cast<TextureBuffer2D*>(value_);
            break;
        case Variable::Type::SamplerCube :
            delete static_cast<CubeMapBuffer*>(value_);
            break;
    }
    value_=nullptr;
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