#pragma once

#include <vector>
#include "thirdparty/glm/glm.hpp"
#include "vir/include/vgraphics/vcore/vshader.h"

namespace ShaderThing
{

struct Uniform : public vir::Shader::Uniform
{
    typedef vir::Shader::Variable::Type Type;
    
    enum class SpecialType
    {
        None,
        Frame,
        RenderPass,
        Time,
        WindowAspectRatio,
        WindowResolution,
        Mouse,
        CameraPosition,
        CameraDirection,
        UserAction,
        LayerAspectRatio,
        LayerResolution,
        Keyboard
    };

    // Uniform special type if this uniform is one of the shared uniforms or if
    // it is a default layer uniform
    SpecialType specialType = SpecialType::None;

    // True if this uniform is shared by all layers
    bool isShared = false;

    struct GUI
    {
        // True if this uniform is of Type::Float3 or Type::Float4 and its value
        // is set via an ImGui color picker tool
        bool usesColorPicker = false;

        // Numerical bounds for the value of this uniform (or its components, if
        // a multi-component vector), only used by uniform types other than
        // Type::Bool, Type::Sampler2D, Type::Cubemap, and not necessarily used
        // by uniforms which have a specialType
        glm::vec2 bounds = {0.f, 1.f};
        
        // True if this uniform's bounds are to be displayed in the GUI
        bool showBounds = true;
    };
    GUI gui;
};

}