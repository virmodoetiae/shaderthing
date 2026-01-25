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

class Uniform : public vir::Uniform
{
protected :

    Uniform() = default;
    DELETE_COPY(Uniform);

public:

    static std::string supportedTypeNames[15];

    static UPtr<Uniform> create()
    {
        return UPtr<Uniform>(new Uniform());
    }

    typedef vir::Uniform::Type Type;

    enum class ManagedType
    {
        None,
        LayerAspectRatio,
        LayerResolution,
    };

    // Further uniform qualifier for automatically-managed uniforms (i.e., 
    // auto-generated uniforms when adding a unfirom wrapping a layer as a
    // resource)
    ManagedType   managedType            = ManagedType::None;
    bool          isSharedByUser         = false;
    bool          hasSharedByUserChanged = false;
    bool          isLogarithmic          = false; // For floats only

    // If a uniform wraps a resource (which can consists of some form of
    // texture 2D/3D texture buffer), it is very convenient to automatically
    // add an additional automatically managed uniform that contains the value
    // of the resolution (W x H or W x H x D) of the wrapped resource. This is
    // what resourceResolutionUniform is for
    UPtr<Uniform> resourceResolutionUniform;
    
    void deleteValue(bool deleteCache) override
    {
        vir::Uniform::deleteValue(deleteCache);
        resourceResolutionUniform.reset();
    }

    void setResourcePtr
    (
        Resource* value, 
        UPtr<vir::DynamicUniformBuffer>& uniformBuffer
    );

    void setResourcePtr
    (
        UPtr<Resource>& value, 
        UPtr<vir::DynamicUniformBuffer>& uniformBuffer
    )   {setResourcePtr(value.get(), uniformBuffer);}

    bool isResource() const;

    struct GUI
    {
        bool markedForDeletion = false;

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