#include <algorithm>
#include <vector>

#include "shaderthing/include/oo/uniform.h"
#include "shaderthing/include/oo/objectio.h"
#include "shaderthing/include/oo/resource.h"
#include "shaderthing/include/structs.h"

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

//----------------------------------------------------------------------------//

const UPtr<Uniform>& Uniform::create(UniformContainer& owner)
{
    auto& u = owner.uniforms.emplace_back(UPtr<Uniform>(new Uniform(owner)));
    owner.uniformBuffer->addUniform(u);
    return u;
}

//----------------------------------------------------------------------------//

Uniform::~Uniform()
{
    if (!isResource())
        return;
    deleteResourceResolutionUniform();
    auto resource = getValuePtr<Resource>();
    if (resource != nullptr && resource->isUsedByUniform(this))
    {
        resource->removeClientUniform(this);
        resource->unbind();
    }
    // The isBeingDestroyed_ flag check is to prevent running std::find on
    // a vector that is actively being destroyed e.g., at program termination
    if (owner_->isBeingDestroyed_ || !owner_->uniformBuffer.valid())
        return;
    auto it = std::find
    (
        owner_->uniforms.begin(), 
        owner_->uniforms.end(), 
        this
    );
    owner_->uniformBuffer->removeUniform(*it);
}

//----------------------------------------------------------------------------//

void Uniform::deleteSelf()
{
    auto it = std::find
    (
        owner_->uniforms.begin(), 
        owner_->uniforms.end(), 
        this
    );
    std::size_t index = std::distance(owner_->uniforms.begin(), it);
    owner_->uniforms.erase(owner_->uniforms.begin()+index);
}

//----------------------------------------------------------------------------//

void Uniform::deleteResourceResolutionUniform()
{
    // The isBeingDestroyed_ flag check is to prevent running std::find on
    // a vector that is actively being destroyed e.g., at program termination
    if 
    (
        isResource() && resourceResolutionUniform_.valid() && 
        !owner_->isBeingDestroyed_
    ) 
    {
        auto it = std::find
        (
            owner_->uniforms.begin(),
            owner_->uniforms.end(),
            resourceResolutionUniform_
        );
        if (it != owner_->uniforms.end())
        {
            std::size_t index = std::distance(owner_->uniforms.begin(), it);
            owner_->uniforms.erase(owner_->uniforms.begin()+index);
        }
    }
}

//----------------------------------------------------------------------------//

void Uniform::deleteValue(bool deleteCache)
{
    vir::Uniform::deleteValue(deleteCache);
    deleteResourceResolutionUniform();
}

//----------------------------------------------------------------------------//

void Uniform::saveToDisk(ObjectIO& io)
{
    if 
    (
        name.size() == 0 || 
        managedType != Uniform::ManagedType::None
    )
        return;

    float& min(gui.bounds.x);
    float& max(gui.bounds.y);
    io.writeObjectStart(name.c_str());
    io.write("type", vir::Shader::uniformTypeToName[type()].c_str());
    io.write("shared", isSharedByUser);

#define WRITE_MIN_MAX       \
    io.write("min", min);   \
    io.write("max", max);

    switch(type())
    {
        case vir::Uniform::Type::Bool :
        {
            io.write("value", getValue<bool>());
            break;
        }
        case vir::Uniform::Type::Int :
        {
            io.write("value", getValue<int>());
            WRITE_MIN_MAX
            break;
        }
        case vir::Uniform::Type::Int2 :
        {
            io.write("value", getValue<glm::ivec2>());
            WRITE_MIN_MAX
            io.write("dragStep", gui.dragStep);
            break;
        }
        case vir::Uniform::Type::Int3 :
        {
            io.write("value", getValue<glm::ivec3>());
            WRITE_MIN_MAX
            break;
        }
        case vir::Uniform::Type::Int4 :
        {
            io.write("value", getValue<glm::ivec4>());
            WRITE_MIN_MAX
            break;
        }
        case vir::Uniform::Type::Float :
        {
            io.write("value", getValue<float>());
            WRITE_MIN_MAX
            break;
        }
        case vir::Uniform::Type::Float2 :
        {
            io.write("value", getValue<glm::vec2>());
            WRITE_MIN_MAX
            io.write("dragStep", gui.dragStep);
            break;
        }
        case vir::Uniform::Type::Float3 :
        {
            io.write("value", getValue<glm::vec3>());
            WRITE_MIN_MAX
            io.write("usesColorPicker", gui.usesColorPicker);
            break;
        }
        case vir::Uniform::Type::Float4 :
        {
            io.write("value", getValue<glm::vec4>());
            WRITE_MIN_MAX
            io.write("usesColorPicker", gui.usesColorPicker);
            break;
        }
        case vir::Uniform::Type::Sampler2D :
        case vir::Uniform::Type::Sampler3D :
        case vir::Uniform::Type::SamplerCube :
        case vir::Uniform::Type::Image2D :
        case vir::Uniform::Type::Image3D :
        case vir::Uniform::Type::ImageCube :
        {
            auto r = getValuePtr<Resource>();
            if (r != nullptr)
                io.write("value", r->name().c_str());
            break;
        }
        default:
            break;
    }
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void Uniform::setType
(
    Type type, 
    uint32_t valueArraySize,
    bool doNotReinitializeIfImageOrSampler,
    bool updateClientBuffers
)
{
    bool typeIsSamplerOrImage2D = 
    (
        this->type() == 
        vir::Uniform::Type::Sampler2D ||
        this->type() == 
        vir::Uniform::Type::Image2D
    );
    bool selectedTypeIsSamplerOrImage2D = 
    (
        type == 
        vir::Uniform::Type::Sampler2D ||
        type == 
        vir::Uniform::Type::Image2D
    );
    bool typeIsSamplerOrImage3D = 
    (
        this->type() == 
        vir::Uniform::Type::Sampler3D ||
        this->type() == 
        vir::Uniform::Type::Image3D
    );
    bool selectedTypeIsSamplerOrImage3D = 
    (
        type == 
        vir::Uniform::Type::Sampler3D ||
        type == 
        vir::Uniform::Type::Image3D
    );
    bool typeIsSamplerOrImageCube = 
    (
        this->type() == 
        vir::Uniform::Type::SamplerCube ||
        this->type() == 
        vir::Uniform::Type::ImageCube
    );
    bool selectedTypeIsSamplerOrImageCube = 
    (
        type == 
        vir::Uniform::Type::SamplerCube ||
        type == 
        vir::Uniform::Type::ImageCube
    );
    bool typeChangedFromResourceToNonResourceType =
    (
        typeIsSamplerOrImage2D ||
        typeIsSamplerOrImage3D ||
        typeIsSamplerOrImageCube
    ) &&
    !(
        selectedTypeIsSamplerOrImage2D ||
        selectedTypeIsSamplerOrImage3D ||
        selectedTypeIsSamplerOrImageCube
    );
    
    // This is only for setting the inUseByLayers_ member of
    // the resource, which in turn is only used to determine
    // whether a full shader recompilation is required
    // after changing the internal format of any resource
    // that is actively used by a layer. This is necessary
    // because, as the choice of using e.g., a 'usampler' or
    // a 'sampler' qualifier for the uniform is automatic,
    // changing the internal uniform type might require
    // changing the qualifier, and this can only be changed
    // in the shader source code with a recompilation
    if (typeChangedFromResourceToNonResourceType)
    {
        auto resource = getValuePtr<Resource>();
        if (resource != nullptr)
            resource->removeClientUniform(this);
    }
    
    vir::Uniform::setType(type, valueArraySize, true, updateClientBuffers);
    gui.showBounds = 
    (
        type != vir::Uniform::Type::Bool &&
        type != vir::Uniform::Type::Sampler2D &&
        type != vir::Uniform::Type::Sampler3D &&
        type != vir::Uniform::Type::SamplerCube &&
        type != vir::Uniform::Type::Image2D &&
        type != vir::Uniform::Type::Image3D &&
        type != vir::Uniform::Type::ImageCube
    );
}

//----------------------------------------------------------------------------//

void Uniform::setType(Type type, bool doNotReinitializeIfImageOrSampler)
{
    setType(type, valueArraySize_, doNotReinitializeIfImageOrSampler, true);
}

//----------------------------------------------------------------------------//

void Uniform::setResourcePtr(const UPtr<Resource>& resource)
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

    vir::Uniform::setValuePtr(resource.get(), type(), false);
    if (resource == nullptr)
        return;

    // Also set resolution uniform
    if (!resourceResolutionUniform_.valid())
    {
        create(*owner_);
        resourceResolutionUniform_ = owner_->uniforms.back().getWeak();
        resourceResolutionUniform_->managedType = 
            ManagedType::ResourceResolution;
    }
    resourceResolutionUniform_->name = name+"Resolution";
    if (is3D)
        resourceResolutionUniform_->setValue
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
        resourceResolutionUniform_->setValue
        (
            glm::vec2
            (
                resource->width(), 
                resource->height()
            ),
            Type::Float2
        );
    // addUniform does nothing if resourceResolutionUniform_ already present, as 
    // it should
}

//----------------------------------------------------------------------------//

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

//----------------------------------------------------------------------------//

void Uniform::setOwner(UniformContainer& owner)
{   
    if (&owner == owner_)
        return;
    // Find UPtr of this Uniform in current owner
    auto it = std::find_if
    (
        owner_->uniforms.begin(), 
        owner_->uniforms.end(), 
        [this](const UPtr<Uniform>& u)
        {
            return u == this;
        }
    );
    std::size_t index = std::distance(owner_->uniforms.begin(), it);
    // Move to new owner's uniforms list and remove from old owner's
    auto& thisUPtr = owner.uniforms.emplace_back
    (
        std::move(owner_->uniforms[index])
    );
    owner_->uniforms.erase(owner_->uniforms.begin()+index);
    // Add to new owner's uniform buffer and remove from old owner's
    owner.uniformBuffer->addUniform(thisUPtr);
    owner_->uniformBuffer->removeUniform(thisUPtr);
    // Repeat for managed resourceResolution if applicable
    if (thisUPtr->isResource() && thisUPtr->resourceResolutionUniform_.valid())
        resourceResolutionUniform_->setOwner(owner);
    // Update owner ptr
    owner_ = &owner;
}

//----------------------------------------------------------------------------//

}
