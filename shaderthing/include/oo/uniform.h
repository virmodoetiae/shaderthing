#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "vir/include/vmacros.h"
#include "vir/include/vgraphics/vcore/vuniform.h"
#include "shaderthing/include/oo/resource.h"
#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

struct UniformContainer
{
friend Uniform;
private:
    bool isBeingDestroyed_ = false;
public:
    virtual ~UniformContainer(){isBeingDestroyed_ = true;}
    UPtrVector<Uniform>             uniforms;
    UPtr<vir::DynamicUniformBuffer> uniformBuffer;
    unsigned int                    uniformBufferBindingPoint;
};

//----------------------------------------------------------------------------//

class Uniform : public vir::Uniform
{
protected :

    Uniform(UniformContainer* owner) : owner_(owner) {};
    DELETE_COPY(Uniform);

    // Owner of this uniform
    UniformContainer* owner_;

    //
    WPtr<Uniform> resourceResolutionUniform_;

    //
    virtual void setType
    (
        Type type, 
        uint32_t valueArraySize,
        bool doNotReinitializeIfImageOrSampler,
        bool updateClientBuffers
    ) override;

    void deleteResourceResolutionUniform();

public:

    static std::string supportedTypeNames[15];

    static const UPtr<Uniform>& create(UniformContainer* owner);

    static const UPtr<Uniform>& create(WPtr<UniformContainer> owner)
    {
        auto* po = owner.get();
        return create(po);
    }

    static void loadAllFrom
    (
        ObjectIO& io, 
        UniformContainer* owner, 
        UPtrVector<Resource>& resource,
        std::map<Uniform*, std::string>& cache
    );

    static void loadAllFrom
    (
        ObjectIO& io, 
        WPtr<UniformContainer> owner,
        UPtrVector<Resource>& resources,
        std::map<Uniform*, std::string>& cache
    )
    {
        auto* po = owner.get();
        loadAllFrom(io, po, resources, cache);
    }

    typedef vir::Uniform::Type Type;

    enum class ManagedType
    {
        None,
        LayerAspectRatio,
        LayerResolution,
        ResourceResolution
    };

    // Further uniform qualifier for automatically-managed uniforms (i.e., 
    // auto-generated uniforms when adding a unfirom wrapping a layer as a
    // resource)
    ManagedType   managedType            = ManagedType::None;
    bool          isSharedByUser         = false;
    bool          isLogarithmic          = false; // For floats only

    ~Uniform();

    void deleteSelf();

    void deleteValue(bool deleteCache) override;

    void saveTo(ObjectIO& io);

    void setType
    (
        Type type, 
        bool doNotReinitializeIfImageOrSampler = false 
    );

    void setResourcePtr(const UPtr<Resource>& value);

    void setOwner(UniformContainer* owner);

    void setOwner(const WPtr<UniformContainer>& owner) {setOwner(owner.get());}

    UniformContainer* owner() const {return owner_;}

    bool isResource() const;

    // If a uniform wraps a resource (which can consists of some form of
    // texture 2D/3D texture buffer), it is very convenient to automatically
    // add a managed uniform that contains the value of the resolution
    // (W x H or W x H x D) of the wrapped resource
    WPtr<Uniform> resourceResolutionUniform() const {return resourceResolutionUniform_;};

    // To enable compatibility with Helpers::enforceUniqueName
    std::string& name() { return vir::Uniform::name; }
    const std::string& name() const { return vir::Uniform::name; }

    struct GUI
    {
        // True if this uniform is of vec3 or vec4 and its value is set via an
        // ImGui color picker tool
        bool usesColorPicker = false;

        // True if this uniform's bounds are to be displayed in the GUI
        bool showBounds = true;

        // Numerical bounds for the value of this uniform (or its components, if
        // a multi-component vector), only used by uniform types other than
        // Type::Bool, Type::Sampler2D, Type::Cubemap, and not necessarily used
        // by uniforms which have a specialType
        glm::vec2 bounds = {0.f, 1.f};

        // Only for vec2, ivec2 type uniforms that can be set by dragging an
        // arrow over the screen. This is a scaling factor from on-screen-arrow
        // size to actual uniform value increment
        float dragStep = 1.;
        
        // For floats only: smallest (absolute) value that can be represented
        // when isLogarithmic == true and the value bounds include 0.f
        float logarithmicZero = 1e-3f;
    };
    GUI gui;
};

}