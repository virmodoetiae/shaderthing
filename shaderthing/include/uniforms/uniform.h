#ifndef ST_UNIFORM_H
#define ST_UNIFORM_H

#include "thirdparty/glm/glm.hpp"
#include "vir/include/vgraphics/vshader.h"

namespace ShaderThing
{

struct Uniform : public vir::Shader::Uniform
{
    // For convenience
    typedef vir::Shader::Variable::Type Type;
    
    //
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
        LayerResolution
    };

    // Uniform special type if this uniform is one of the shared uniforms or if
    // it is a default layer uniform
    SpecialType specialType = SpecialType::None;

    // True if this uniform is shared by all layers
    bool isShared = false;

    // True if this uniform is of Type::Float3 or Type::Float4 and its value is
    // set via an ImGui color picker tool
    bool usesColorPicker = false;

    // True if this uniform is of Type::Int3 and its value tracks the status
    // of a select keyboard key (down, pressed, toggled)
    bool usesKeyboardInput = false;

    // Keycode of the tracked keyboard input if usesKeyboardInput is true
    int keyCode = -1;

    // 
    const bool* keyState[3] = {nullptr, nullptr, nullptr};

    // Numerical limits for the value of this uniform (or its components, if a
    // multi-component vector), only used by uniform types other than
    // Type::Bool, Type::Sampler2D, Type::Cubemap, and not necessarily used by
    // uniforms which have a specialType
    glm::vec2 limits = {0.f, 1.f};
    
    // True if this uniform's limits are to be displayed in the GUI
    bool showLimits = true;
};

}

#endif