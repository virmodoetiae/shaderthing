#include <algorithm>
#include <vector>

#include "shaderthing/include/oo/uniform.h"
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
    (void)this;
    if (!isResource())
        return;
    // Delete resource resolution uniform
    if (resourceResolutionUniform_.valid())
    {
        owner_->uniformBuffer->removeUniform(resourceResolutionUniform_);
        auto it = std::find
        (
            owner_->uniforms.begin(), 
            owner_->uniforms.end(), 
            resourceResolutionUniform_
        );
        std::size_t index = std::distance(owner_->uniforms.begin(), it);
        owner_->uniforms.erase(owner_->uniforms.begin()+index);
    }
    auto resource = getValuePtr<Resource>();
    if (resource != nullptr && resource->isUsedByUniform(this))
    {
        resource->removeClientUniform(this);
        resource->unbind();
    }
}

//----------------------------------------------------------------------------//

void Uniform::deleteSelf()
{
    auto it = std::find_if
    (
        owner_->uniforms.begin(), 
        owner_->uniforms.end(), 
        [this](const UPtr<Uniform>& u)
        {
            return u == this;
        }
    );
    owner_->uniformBuffer->removeUniform(*it);
    std::size_t index = std::distance(owner_->uniforms.begin(), it);
    owner_->uniforms.erase(owner_->uniforms.begin()+index);
}

//----------------------------------------------------------------------------//

void Uniform::deleteValue(bool deleteCache)
{
    vir::Uniform::deleteValue(deleteCache);

    // Also reset resource resolution uniform
    if (isResource() && resourceResolutionUniform_.valid()) 
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
    bool typeChangedFromNonResourceToResource =
        !(
            typeIsSamplerOrImage2D ||
            typeIsSamplerOrImage3D ||
            typeIsSamplerOrImageCube
        ) &&
        (
            selectedTypeIsSamplerOrImage2D ||
            selectedTypeIsSamplerOrImage3D ||
            selectedTypeIsSamplerOrImageCube
        );
    bool typeChangedFromResourceToIncompatibleResource = 
        (
            typeIsSamplerOrImage2D && 
            (
                selectedTypeIsSamplerOrImage3D || 
                selectedTypeIsSamplerOrImageCube
            )
        ) ||
        (
            typeIsSamplerOrImage3D && 
            (
                selectedTypeIsSamplerOrImage2D || 
                selectedTypeIsSamplerOrImageCube
            )
        ) ||
        (
            typeIsSamplerOrImageCube && 
            (
                selectedTypeIsSamplerOrImage2D || 
                selectedTypeIsSamplerOrImage3D
            )
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
    // TODO - CHECK BIG CHANGE
    //else if (typeChangedFromNonResourceToResource)
    //    uniform->removeFromAllClientBuffers();
    
    //if 
    //(
    //    typeChangedFromResourceToNonResourceType ||
    //    typeChangedFromResourceToIncompatibleResource
    //)
        /*
        // TODO: Check if madking an ad-hoc function for this in 
        // ShaderThing::Uniform is cleaner
        layer->rendering.uniformBuffer->removeUniform
        (
            uniform->resourceResolutionUniform
        );
        */
        //uniform->resourceResolutionUniform()->
        //   removeFromAllClientBuffers();
    
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

    // Add uniform to the buffer if it is a non-resource type now
    // that the type has been set (cannot do it before, as I need
    // to have the new uniform type already set before adding the
    // uniform the buffer)

    if 
    (
            selectedTypeIsSamplerOrImage2D ||
            selectedTypeIsSamplerOrImage3D ||
            selectedTypeIsSamplerOrImageCube
    )
        return;
    
    // TODO: Check if not needed (already managed when adding/removing
    // uniforms in the buffer itself, right?)
    // layer->uniformBuffer->recalculateUniformSizesAndOffsets();
    // uniform->markForSubmissionToAllClientBuffers();
}

//----------------------------------------------------------------------------//

void Uniform::setType(Type type, bool doNotReinitializeIfImageOrSampler)
{
    setType(type, valueArraySize_, doNotReinitializeIfImageOrSampler, true);
}

//----------------------------------------------------------------------------//

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
    uniformBuffer->addUniform(resourceResolutionUniform_);
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
