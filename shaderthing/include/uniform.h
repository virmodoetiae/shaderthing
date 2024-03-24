/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <vector>

#include "shaderthing/include/macros.h"

#include "vir/include/vgraphics/vcore/vshader.h"

#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

class Layer;
class ObjectIO;
class Resource;
class SharedUniforms;

class Uniform : public vir::Shader::Uniform
{
public:

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
    SpecialType specialType            = SpecialType::None;

    bool        isSharedByUser         = false;
    bool        hasSharedByUserChanged = false;

    struct GUI
    {
        bool markedForDeletion = false;

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

    DELETE_COPY_MOVE(Uniform)

    // Render the Uniforms tab bar GUI, which allows to add/remove/modify
    // existing layer uniforms and shared uniforms. It also sets the uniform
    // values in the provided layer shader for convenience: the GUI has 
    // knowledge of when a uniform changes, and it is only then that the 
    // uniform value is actually set (once) in the shader. 
    static void renderUniformsGui
    (
        SharedUniforms& sharedUniforms,
        Layer* layer,
        const std::vector<Layer*>& layers,
        const std::vector<Resource*>& resources
    );

    static void loadAll
    (
        const ObjectIO& io, 
        std::vector<Uniform*>& uniforms,
        const std::vector<Resource*>& resources,
        std::map<Uniform*, std::string>& uninitializedResourceLayers
    );
    static void saveAll(ObjectIO& io, const std::vector<Uniform*>& uniforms);
};

}