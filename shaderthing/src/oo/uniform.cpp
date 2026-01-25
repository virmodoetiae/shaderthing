#include "shaderthing/include/oo/uniform.h"
#include "shaderthing/include/oo/resource.h"

namespace ShaderThing
{

std::string Uniform::supportedTypeNames[15] =
{
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Bool],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Int],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Int2],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Int3],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Int4],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Float],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Float2],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Float3],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Float4],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Sampler2D],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Sampler3D],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::SamplerCube],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Image2D],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::Image3D],
    vir::Shader::uniformTypeToName[vir::Uniform::Type::ImageCube]
};

void Uniform::setResourcePtr
(
    Resource* resource, 
    UPtr<vir::DynamicUniformBuffer>& uniformBuffer
)
{
    bool is3D;
    switch(type())
    {
        case vir::Uniform::Type::Sampler2D :
        case vir::Uniform::Type::Image2D :
        case vir::Uniform::Type::SamplerCube :
        case vir::Uniform::Type::ImageCube :
            is3D = false;
            break;
        case vir::Uniform::Type::Sampler3D :
        case vir::Uniform::Type::Image3D :
            is3D = true;
            break;
        default :
            return;
    }

    vir::Uniform::setValuePtr(resource, type(), false);
    if (resource == nullptr)
        return;

    // Also set resolution uniform
    if (resourceResolutionUniform == nullptr)
        resourceResolutionUniform = Uniform::create();
    resourceResolutionUniform->name = name+"Resolution";
    if (is3D)
        resourceResolutionUniform->setValue
        (
            glm::vec3
            (
                resource->width(), 
                resource->height(),
                resource->depth()
            ),
            Type::Float3
        );
    else
        resourceResolutionUniform->setValue
        (
            glm::vec2
            (
                resource->width(), 
                resource->height()
            ),
            Type::Float2
        );
    //if (uniformBuffer != nullptr)
    uniformBuffer->addUniform(resourceResolutionUniform);
}

bool Uniform::isResource() const 
{
    switch(type())
    {
        case vir::Uniform::Type::Sampler2D :
        case vir::Uniform::Type::Image2D :
        case vir::Uniform::Type::SamplerCube :
        case vir::Uniform::Type::ImageCube :
        case vir::Uniform::Type::Sampler3D :
        case vir::Uniform::Type::Image3D :
            return true;
        default :
            return false;
    }
}

}
