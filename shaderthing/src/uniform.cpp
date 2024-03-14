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

#include "shaderthing/include/uniform.h"

#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/shareduniforms.h"

#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

bool Uniform::renderUniformsGui
(
    SharedUniforms& sharedUniforms,
    std::vector<Uniform*>& uniforms,
    std::vector<Uniform*>& uncompiledUniforms,
    vir::Shader& shader,
    const std::vector<Resource*>& resources
)
{
    const float fontSize = ImGui::GetFontSize();
    int row = 0; 
    #define START_ROW                                                       \
        ImGui::PushID(row);                                                 \
        ImGui::TableNextRow(0, 1.6*fontSize);                               \
        column = 0;
    #define END_ROW                                                         \
        ImGui::PopID();                                                     \
        ++row;
    #define START_COLUMN                                                    \
        ImGui::TableSetColumnIndex(column);                                 \
        ImGui::PushItemWidth(-1);
    #define END_COLUMN                                                      \
        ++column;                                                           \
        ImGui::PopItemWidth();
    #define NEXT_COLUMN                                                     \
        ImGui::TableSetColumnIndex(column++);

    //--------------------------------------------------------------------------
    auto renderEditUniformBoundsButtonGui =
    [&fontSize](Type type, glm::vec2& bounds)
    {
        if (ImGui::Button(ICON_FA_RULER_COMBINED, ImVec2(-1, 0)))
            ImGui::OpenPopup("##uniformBounds");
        if (ImGui::BeginPopup("##uniformBounds"))
        {
            
            glm::vec2 bounds0(bounds);
            if (type == vir::Shader::Variable::Type::UInt)
                bounds.x = std::max(bounds.x, 0.0f);
            ImGui::Text("Minimum value ");
            ImGui::SameLine();
            float inputWidth = 6*fontSize;
            auto minf = Helpers::getFormat(bounds0.x);
            auto maxf = Helpers::getFormat(bounds0.y);
            ImGui::PushItemWidth(inputWidth);
            ImGui::InputFloat
            (
                "##minValueInput", 
                &(bounds.x), 0.f, 0.f,
                minf.c_str()
            );
            ImGui::PopItemWidth();
            ImGui::Text("Maximum value ");
            ImGui::SameLine();
            ImGui::PushItemWidth(inputWidth);
            ImGui::InputFloat
            (
                "##maxValueInput", 
                &(bounds.y), 0.f, 0.f,
                maxf.c_str()
            );
            ImGui::PopItemWidth();
            ImGui::EndPopup();
            return (bounds != bounds0); 
        }
        return false;
    }; // End of renderUniformBoundsButtonGui lambda

    //--------------------------------------------------------------------------
    auto renderSharedUniformsGui = 
    [&fontSize, &renderEditUniformBoundsButtonGui]
    (SharedUniforms& sharedUniforms, int& row)
    {
        int column;
        float halfButtonSize(1.7*fontSize);

        // iFrame --------------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if (ImGui::Button(ICON_FA_UNDO, ImVec2(halfButtonSize, 0)))
        {
            sharedUniforms.resetFrameCounter();
            Layer::Flags::restartRendering = true;
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
            ImGui::BeginTooltip()
        )
        {
            static ImVec4 ctrlRColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            ImGui::Text("Restart rendering");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ctrlRColor);
            ImGui::Text("Ctrl+R");
            ImGui::PopStyleColor();
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        // Pause/resume rendering, which also affects iTime (but the
        // opposite is not true)
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isRenderingPaused ? 
                ICON_FA_PLAY : 
                ICON_FA_PAUSE, 
                ImVec2(-1, 0)
            )
        )
        {
            sharedUniforms.flags_.isRenderingPaused = 
                !sharedUniforms.flags_.isRenderingPaused;
            sharedUniforms.flags_.isTimePaused = 
                sharedUniforms.flags_.isRenderingPaused;
        }
        NEXT_COLUMN
        ImGui::Text("iFrame");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::UInt].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text("%d", sharedUniforms.fBlock_.iFrame);
        END_ROW

        // iTime --------------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isTimeLooped ?
                ICON_FA_INFINITY : 
                ICON_FA_CIRCLE_NOTCH,
                ImVec2(halfButtonSize, 0)
            )
        )
            sharedUniforms.flags_.isTimeLooped = 
                !sharedUniforms.flags_.isTimeLooped;
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text
            (
                sharedUniforms.flags_.isTimeLooped ? 
                "Disable loop" : 
                "Enable loop"
            );
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isTimePaused ? 
                ICON_FA_PLAY : 
                ICON_FA_PAUSE, 
                ImVec2(-1, 0)
            ) && !sharedUniforms.flags_.isRenderingPaused
        )
            sharedUniforms.flags_.isTimePaused = 
                !sharedUniforms.flags_.isTimePaused;
        NEXT_COLUMN
        ImGui::Text("iTime");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
        NEXT_COLUMN
        glm::vec2* bounds = &sharedUniforms.bounds_[SpecialType::Time];
        bool boundsChanged = renderEditUniformBoundsButtonGui
        (
            Type::Float, 
            sharedUniforms.bounds_[SpecialType::Time]
        );
        NEXT_COLUMN
        auto iTimePtr = &sharedUniforms.fBlock_.iTime;
        if (!boundsChanged)
        {
            bounds->x = std::min(*iTimePtr, bounds->x);
            bounds->y = std::max(*iTimePtr, bounds->y);
        }
        ImGui::PushItemWidth(-1);
        if 
        (
            ImGui::SliderFloat
            (
                "##iTimeSlider", 
                iTimePtr, 
                bounds->x,
                bounds->y,
                "%.3f"
            ) || boundsChanged
        )
        {
            if (boundsChanged)
            {
                *iTimePtr = std::max(*iTimePtr, bounds->x);
                *iTimePtr = std::min(*iTimePtr, bounds->y);
            }
            sharedUniforms.setUserAction(true);
        }
        ImGui::PopItemWidth();
        END_ROW

        START_ROW
        NEXT_COLUMN
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isTimeResetOnFrameCounterReset ? 
                ICON_FA_BAN " " ICON_FA_UNDO: 
                ICON_FA_CHECK " " ICON_FA_UNDO, 
                ImVec2(-1, 0)
            )
        )
            sharedUniforms.flags_.isTimeResetOnFrameCounterReset =
                !sharedUniforms.flags_.isTimeResetOnFrameCounterReset;
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            if (sharedUniforms.flags_.isTimeResetOnFrameCounterReset)
                ImGui::Text("Disable time reset on rendering restart");
            else
                ImGui::Text("Enable time reset on rendering restart");
            ImGui::EndTooltip();
        }
        END_ROW

        // iTimeDelta --------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isTimeDeltaSmooth ?
                ICON_FA_WAVE_SQUARE : 
                ICON_FA_SIGNATURE, 
                ImVec2(-1, 0)
            )
        )
            sharedUniforms.flags_.isTimeDeltaSmooth =
                !sharedUniforms.flags_.isTimeDeltaSmooth;
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            if (sharedUniforms.flags_.isTimeDeltaSmooth)
                ImGui::Text("Disable time step smoothing");
            else
                ImGui::Text("Enable time step smoothing");
            ImGui::EndTooltip();
        }
        // No actions
        NEXT_COLUMN
        ImGui::Text("iTimeDelta");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text("%.4f s", sharedUniforms.fBlock_.iTimeDelta);
        END_ROW
        ImGui::Dummy({0, 0.1f*fontSize});

        // iWindowAspectRatio --------------------------------------------------
        START_ROW
        NEXT_COLUMN
        // No actions
        NEXT_COLUMN
        ImGui::Text("iWindowAspectRatio");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text
        (
            "%.3f", sharedUniforms.fBlock_.iAspectRatio
        );
        END_ROW
        ImGui::Dummy({0, 0.1f*fontSize});

        // iWindowResolution ---------------------------------------------------
        START_ROW
        NEXT_COLUMN
        // No actions
        NEXT_COLUMN
        ImGui::Text("iWindowResolution");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Int2].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text
        (
            "%d x %d", 
            (int)sharedUniforms.fBlock_.iResolution.x, 
            (int)sharedUniforms.fBlock_.iResolution.y
        );
        END_ROW
        
        // iKeyboard -----------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isKeyboardInputEnabled ? 
                ICON_FA_PAUSE : 
                ICON_FA_PLAY, 
                ImVec2(-1, 0)
            )
        )
            sharedUniforms.toggleKeyboardInputs();
        NEXT_COLUMN
        ImGui::Text("iKeyboard");
        NEXT_COLUMN
        ImGui::Text("vec3[]");
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        std::string pressed = "Pressed:";
        std::string held    = "Held:   ";
        std::string toggled = "Toggled:";
        for (int key=0; key<255; key++)
        {
            auto& keyData(sharedUniforms.fBlock_.iKeyboard[key]);
            if (keyData.x > 0)
                pressed += " "+vir::keyCodeToName[key];
            else if (keyData.y > 0)
                held += " "+vir::keyCodeToName[key];
            if (keyData.z > 0)
                toggled += " "+vir::keyCodeToName[key];
        }
        ImGui::Text(pressed.c_str());
        ImGui::Dummy({0, 0.1f*fontSize});
        ImGui::Text(held.c_str());
        ImGui::Dummy({0, 0.1f*fontSize});
        ImGui::Text(toggled.c_str());
        ImGui::Dummy({0, 0.1f*fontSize});
        END_ROW
        
        
        // iMouse --------------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isMouseInputEnabled ? 
                ICON_FA_PAUSE : 
                ICON_FA_PLAY, 
                ImVec2(-1, 0)
            )
        )
            sharedUniforms.toggleMouseInputs();
        NEXT_COLUMN
        ImGui::Text("iMouse");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float4].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text
        (
            "%d, %d, %d, %d", 
            (int)sharedUniforms.fBlock_.iMouse.x, 
            (int)sharedUniforms.fBlock_.iMouse.y, 
            (int)sharedUniforms.fBlock_.iMouse.z, 
            (int)sharedUniforms.fBlock_.iMouse.w
        );
        END_ROW
        
        
        // iWASD ---------------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
            ImGui::OpenPopup("##iWASDSettings");
        if (ImGui::BeginPopup("##iWASDSettings"))
        {
            bool enabled = sharedUniforms.flags_.isCameraKeyboardInputEnabled;
            std::string text = enabled ? "Disable" : "Enable";
            if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
                sharedUniforms.toggleCameraKeyboardInputs();
            ImGui::Text("Keyboard sensitivity ");
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat
            (
                "##iWASDSensitivity", 
                &sharedUniforms.shaderCamera_->keySensitivityRef(),
                1e-1,
                50
            );
            ImGui::PopItemWidth();
            ImGui::EndPopup();
        }
        NEXT_COLUMN
        ImGui::Text("iWASD");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
        NEXT_COLUMN
        bounds = &sharedUniforms.bounds_[SpecialType::CameraPosition];
        boundsChanged = renderEditUniformBoundsButtonGui
        (
            Type::Float3, 
            sharedUniforms.bounds_[SpecialType::CameraPosition]
        );
        NEXT_COLUMN
        {
            glm::vec3 value = sharedUniforms.fBlock_.iWASD.packed();
            std::string format = Helpers::getFormat(value);
            if (!boundsChanged)
            {
                bounds->x = std::min(value.x, bounds->x);
                bounds->x = std::min(value.y, bounds->x);
                bounds->x = std::min(value.z, bounds->x);
                bounds->y = std::max(value.x, bounds->y);
                bounds->y = std::max(value.y, bounds->y);
                bounds->y = std::max(value.z, bounds->y);
            }
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::SliderFloat3
                (
                    "##iWASDSlider", 
                    glm::value_ptr(value), 
                    bounds->x,
                    bounds->y,
                    format.c_str()
                ) || boundsChanged
            )
            {
                if (boundsChanged)
                {
                    value.x = std::max(value.x, bounds->x);
                    value.x = std::min(value.x, bounds->y);
                    value.y = std::max(value.y, bounds->x);
                    value.y = std::min(value.y, bounds->y);
                    value.z = std::max(value.z, bounds->x);
                    value.z = std::min(value.z, bounds->y);
                }
                sharedUniforms.fBlock_.iWASD = value;
                sharedUniforms.shaderCamera_->setPosition(value);
                sharedUniforms.setUserAction(true);
            }
            ImGui::PopItemWidth();
        }
        END_ROW

        // iLook ---------------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
            ImGui::OpenPopup("##iLookSettings");
        if (ImGui::BeginPopup("##iLookSettings"))
        {
            bool enabled = sharedUniforms.flags_.isCameraMouseInputEnabled;
            std::string text = enabled ? "Disable" : "Enable";
            if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
                sharedUniforms.toggleCameraMouseInputs();
            ImGui::Text("Mouse sensitivity ");
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat
            (
                "##iLookSensitivity", 
                &sharedUniforms.shaderCamera_->mouseSensitivityRef(),
                1e-3,
                1
            );
            ImGui::PopItemWidth();
            ImGui::EndPopup();
        }
        NEXT_COLUMN
        ImGui::Text("iLook");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
        NEXT_COLUMN
        // All cmpts always bounds in [-1, 1]
        NEXT_COLUMN
        {
            glm::vec3 value = sharedUniforms.fBlock_.iLook.packed();
            std::string format = Helpers::getFormat(value);
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::SliderFloat3
                (
                    "##iLookSlider", 
                    glm::value_ptr(value), 
                    -1,
                    1,
                    format.c_str()
                )
            )
            {
                value = glm::normalize(value);
                sharedUniforms.fBlock_.iLook = value;
                sharedUniforms.shaderCamera_->setDirection(value);
                sharedUniforms.setUserAction(true);
            }
            ImGui::PopItemWidth();
        }
        END_ROW
        ImGui::Dummy({0, 0.05f*fontSize});
    }; // End of renderSharedUniformsGui lambda

    // -------------------------------------------------------------------------
    static std::string supportedUniformTypeNames[11]
    {
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Bool],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int2],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int3],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int4],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float2],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float3],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float4],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Sampler2D],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::SamplerCube]
    };
    auto renderUniformGui = 
    [&fontSize, &renderEditUniformBoundsButtonGui]
    (
        SharedUniforms& sharedUniforms,
        Uniform* uniform,
        std::vector<Uniform*>& uncompiledUniforms,
        vir::Shader& shader,
        const std::vector<Resource*>& resources,
        int& row
    )
    {
        int column;
        bool managed
        (
            uniform->specialType == SpecialType::LayerAspectRatio ||
            uniform->specialType == SpecialType::LayerResolution
        );
        auto name0 = uniform->name;
        auto type0 = uniform->type;
        
        START_ROW

        START_COLUMN // Action column ------------------------------------------
        if (!managed)
        {
            if (ImGui::Button(ICON_FA_TRASH, ImVec2(-1, 0)))
                uniform->gui.markedForDeletion = true;
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text("Delete this uniform");
                ImGui::EndTooltip();
            }
        }
        END_COLUMN
        
        START_COLUMN // Name column --------------------------------------------
        if (managed)
            ImGui::Text(uniform->name.c_str());
        else
            ImGui::InputText("##uniformName", &uniform->name);
        bool named(uniform->name.size() > 0);
        END_COLUMN

        START_COLUMN // Type column --------------------------------------------
        if (managed)
            ImGui::Text(vir::Shader::uniformTypeToName[uniform->type].c_str());
        else if 
        (
            ImGui::BeginCombo
            (
                "##ufSelector", 
                vir::Shader::uniformTypeToName[uniform->type].c_str()
            )
        )
        {
            for(auto uniformTypeName : supportedUniformTypeNames)
                if (ImGui::Selectable(uniformTypeName.c_str()))
                {
                    auto selectedType = 
                        vir::Shader::uniformNameToType[uniformTypeName];
                    if (selectedType != uniform->type)
                    {
                        uniform->resetValue();
                        uniform->type = selectedType;
                        uniform->gui.showBounds = 
                        (
                            selectedType != 
                                vir::Shader::Variable::Type::Bool &&
                            selectedType != 
                                vir::Shader::Variable::Type::Sampler2D &&
                            selectedType != 
                                vir::Shader::Variable::Type::SamplerCube
                        );
                    }
                }
            ImGui::EndCombo();
        }
        END_COLUMN

        START_COLUMN // Bounds column ------------------------------------------
        bool boundsChanged(false);
        glm::vec2& bounds = uniform->gui.bounds;
        if (uniform->gui.showBounds)
        {
            boundsChanged = renderEditUniformBoundsButtonGui
            (
                uniform->type, 
                bounds
            );
        }
        
        END_COLUMN

        START_COLUMN // Value column -------------------------------------------
        switch(uniform->type)
        {
            case vir::Shader::Variable::Type::Bool :
            {
                auto value = uniform->getValue<bool>();
                if (ImGui::Checkbox((value) ? "true" : "false", &value))
                {
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformBool(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::UInt : //-------------------------
            {
                auto value = uniform->getValue<uint32_t>();
                value = std::max(value, (uint32_t)0);
                if (!boundsChanged)
                {
                    bounds.x = std::min((float)value, bounds.x);
                    bounds.y = std::max((float)value, bounds.y);
                }
                bool input = ImGui::SliderInt
                (
                    "##uniformSliderInt", 
                    (int*)&value, 
                    (int)(bounds.x),
                    (int)(bounds.y)
                );
                if (input || boundsChanged)
                {
                    value = std::max(value, (uint32_t)0);
                    if (boundsChanged)
                    {
                        value = std::max((float)value, bounds.x);
                        value = std::min((float)value, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int : //--------------------------
            {
                auto value = uniform->getValue<int>();
                if (!boundsChanged)
                {
                    bounds.x = std::min((float)value, bounds.x);
                    bounds.y = std::max((float)value, bounds.y);
                }
                bool input(false);
                if 
                (
                    ImGui::SliderInt
                    (
                        "##iSlider", 
                        &value, 
                        (int)(bounds.x),
                        (int)(bounds.y)
                    ) || boundsChanged
                )
                {
                    if (boundsChanged)
                    {
                        value = std::max((float)value, bounds.x);
                        value = std::min((float)value, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int2 : //-------------------------
            {
                auto value = uniform->getValue<glm::ivec2>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, (int)bounds.x);
                    bounds.x = std::min(value.y, (int)bounds.x);
                    bounds.y = std::max(value.x, (int)bounds.y);
                    bounds.y = std::max(value.y, (int)bounds.y);
                }
                bool input(false);
                {
                    ImGui::SmallButton(ICON_FA_MOUSE_POINTER); 
                    ImGui::SameLine();
                    if (ImGui::IsItemActive())
                    {
                        ImGui::GetForegroundDrawList()->AddLine
                        (
                            ImGui::GetIO().MouseClickedPos[0], 
                            ImGui::GetIO().MousePos, 
                            ImGui::GetColorU32(ImGuiCol_Button), 
                            4.0f
                        );
                        ImVec2 delta = ImGui::GetMouseDragDelta(0, 0.0f);
                        delta.y = -delta.y;
                        auto monitor = 
                            vir::Window::instance()->
                            primaryMonitorResolution();
                        int maxRes = std::max(monitor.x, monitor.y);
                        bounds.x = std::min(bounds.x, -bounds.y);
                        bounds.y = std::max(-bounds.x, bounds.y);
                        value.x = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.x*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        value.y = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.y*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        input = true;
                    }
                    bool input2 = ImGui::SliderInt2
                    (
                        "##i2Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y
                    );
                    input = input || input2;
                }
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, (int)bounds.x);
                        value.x = std::min(value.x, (int)bounds.y);
                        value.y = std::max(value.y, (int)bounds.x);
                        value.y = std::min(value.y, (int)bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt2(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int3 : //-------------------------
            {
                auto value = uniform->getValue<glm::ivec3>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, (int)bounds.x);
                    bounds.x = std::min(value.y, (int)bounds.x);
                    bounds.x = std::min(value.z, (int)bounds.x);
                    bounds.y = std::max(value.x, (int)bounds.y);
                    bounds.y = std::max(value.y, (int)bounds.y);
                    bounds.y = std::max(value.z, (int)bounds.y);
                }
                if 
                (
                    ImGui::SliderInt3
                    (
                        "##i3Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y
                    )
                )
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, (int)bounds.x);
                        value.x = std::min(value.x, (int)bounds.y);
                        value.y = std::max(value.y, (int)bounds.x);
                        value.y = std::min(value.y, (int)bounds.y);
                        value.z = std::max(value.z, (int)bounds.x);
                        value.z = std::min(value.z, (int)bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt3
                        (
                            uniform->name, 
                            value
                        );
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int4 : //-------------------------
            {
                auto value = uniform->getValue<glm::ivec4>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, (int)bounds.x);
                    bounds.x = std::min(value.y, (int)bounds.x);
                    bounds.x = std::min(value.z, (int)bounds.x);
                    bounds.x = std::min(value.w, (int)bounds.x);
                    bounds.y = std::max(value.x, (int)bounds.y);
                    bounds.y = std::max(value.y, (int)bounds.y);
                    bounds.y = std::max(value.z, (int)bounds.y);
                    bounds.y = std::max(value.w, (int)bounds.y);
                }
                if 
                (
                    ImGui::SliderInt4
                    (
                        "##i4Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y
                    )
                )
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, (int)bounds.x);
                        value.x = std::min(value.x, (int)bounds.y);
                        value.y = std::max(value.y, (int)bounds.x);
                        value.y = std::min(value.y, (int)bounds.y);
                        value.z = std::max(value.z, (int)bounds.x);
                        value.z = std::min(value.z, (int)bounds.y);
                        value.w = std::max(value.z, (int)bounds.x);
                        value.w = std::min(value.z, (int)bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt4(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float : //------------------------
            {
                auto value = uniform->getValue<float>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value, bounds.x);
                    bounds.y = std::max(value, bounds.y);
                }
                bool input(false);
                if 
                (
                    uniform->specialType == 
                        Uniform::SpecialType::WindowAspectRatio ||
                    uniform->specialType == 
                        Uniform::SpecialType::LayerAspectRatio
                )
                    ImGui::Text("%.3f", value);
                else
                {
                    std::string format = Helpers::getFormat(value);
                    input = ImGui::SliderFloat
                    (
                        "##fSlider", 
                        &value, 
                        bounds.x,
                        bounds.y,
                        format.c_str()
                    );
                }
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value = std::max(value, bounds.x);
                        value = std::min(value, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformFloat(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float2 : //-----------------------
            {
                auto value = uniform->getValue<glm::vec2>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, bounds.x);
                    bounds.x = std::min(value.y, bounds.x);
                    bounds.y = std::max(value.x, bounds.y);
                    bounds.y = std::max(value.y, bounds.y);
                }
                bool input(false);
                if 
                (
                    uniform->specialType == 
                        Uniform::SpecialType::WindowResolution || 
                    uniform->specialType == 
                        Uniform::SpecialType::LayerResolution
                )
                {
                    auto ivalue = uniform->getValue<glm::ivec2>();
                    ImGui::Text
                    (
                        "%d x %d",
                        ivalue.x, ivalue.y
                    );
                }
                else 
                {
                    ImGui::SmallButton(ICON_FA_MOUSE_POINTER); 
                    ImGui::SameLine();
                    if (ImGui::IsItemActive())
                    {
                        ImGui::GetForegroundDrawList()->AddLine
                        (
                            ImGui::GetIO().MouseClickedPos[0], 
                            ImGui::GetIO().MousePos, 
                            ImGui::GetColorU32(ImGuiCol_Button), 
                            4.0f
                        );
                        ImVec2 delta = ImGui::GetMouseDragDelta(0, 0.0f);
                        delta.y = -delta.y;
                        auto monitor = 
                            vir::Window::instance()->
                            primaryMonitorResolution();
                        int maxRes = std::max(monitor.x, monitor.y);
                        bounds.x = std::min(bounds.x, -bounds.y);
                        bounds.y = std::max(-bounds.x, bounds.y);
                        value.x = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.x*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        value.y = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.y*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        input = true;
                    }
                    std::string format = Helpers::getFormat(value);
                    bool input2 = ImGui::SliderFloat2
                    (
                        "##f2Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y,
                        format.c_str()
                    );
                    input = input || input2;
                }
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, bounds.x);
                        value.x = std::min(value.x, bounds.y);
                        value.y = std::max(value.y, bounds.x);
                        value.y = std::min(value.y, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformFloat2(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float3 : //-----------------------
            {
                auto value = uniform->getValue<glm::vec3>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, bounds.x);
                    bounds.x = std::min(value.y, bounds.x);
                    bounds.x = std::min(value.z, bounds.x);
                    bounds.y = std::max(value.x, bounds.y);
                    bounds.y = std::max(value.y, bounds.y);
                    bounds.y = std::max(value.z, bounds.y);
                }
                bool colorPicker(false);
                if 
                (
                    ImGui::SmallButton
                    (
                        uniform->gui.usesColorPicker ? 
                        ICON_FA_SLIDERS_H : 
                        ICON_FA_PAINT_BRUSH
                    )
                )
                    uniform->gui.usesColorPicker = 
                        !uniform->gui.usesColorPicker;
                ImGui::SameLine();
                colorPicker = uniform->gui.usesColorPicker;
                if (!colorPicker)
                {
                    uniform->gui.showBounds = true;
                    std::string format = Helpers::getFormat(value);
                    if 
                    (
                        ImGui::SliderFloat3
                        (
                            "##f3Slider", 
                            glm::value_ptr(value), 
                            bounds.x,
                            bounds.y,
                            format.c_str()
                        ) || boundsChanged
                    )
                    {
                        if (boundsChanged)
                        {
                            value.x = std::max(value.x, bounds.x);
                            value.x = std::min(value.x, bounds.y);
                            value.y = std::max(value.y, bounds.x);
                            value.y = std::min(value.y, bounds.y);
                            value.z = std::max(value.z, bounds.x);
                            value.z = std::min(value.z, bounds.y);
                        }
                        bool isCameraDirection
                        (
                            uniform->specialType == 
                                Uniform::SpecialType::CameraDirection
                        );
                        if (isCameraDirection)
                            value = glm::normalize(value);
                        uniform->setValue(value);
                        if (named)
                        {
                            shader.setUniformFloat3(uniform->name, value);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                else
                {
                    uniform->gui.showBounds = false;
                    if 
                    (
                        ImGui::ColorEdit3
                        (
                            "##uniformColorEdit3", 
                            glm::value_ptr(value)
                        )
                    )
                    {
                        bounds.x = 0.0;
                        bounds.y = 1.0;
                        uniform->setValue(value);
                        if (named)
                        {
                            shader.setUniformFloat3(uniform->name, value);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float4 : //-----------------------
            {
                auto value = uniform->getValue<glm::vec4>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, bounds.x);
                    bounds.x = std::min(value.y, bounds.x);
                    bounds.x = std::min(value.z, bounds.x);
                    bounds.x = std::min(value.w, bounds.x);
                    bounds.y = std::max(value.x, bounds.y);
                    bounds.y = std::max(value.y, bounds.y);
                    bounds.y = std::max(value.z, bounds.y);
                    bounds.y = std::max(value.w, bounds.y);
                }
                bool colorPicker(false);
                if 
                (
                    ImGui::SmallButton
                    (
                        uniform->gui.usesColorPicker ? 
                        ICON_FA_SLIDERS_H : 
                        ICON_FA_PAINT_BRUSH
                    )
                )
                    uniform->gui.usesColorPicker = 
                        !uniform->gui.usesColorPicker;
                ImGui::SameLine();
                colorPicker = uniform->gui.usesColorPicker;
                if (!colorPicker)
                {
                    uniform->gui.showBounds = true;
                    std::string format = Helpers::getFormat(value);
                    if 
                    (
                        ImGui::SliderFloat4
                        (
                            "##f4Slider", 
                            glm::value_ptr(value), 
                            bounds.x,
                            bounds.y,
                            format.c_str()
                        ) || boundsChanged
                    )
                    {
                        if (boundsChanged)
                        {
                            value.x = std::max(value.x, bounds.x);
                            value.x = std::min(value.x, bounds.y);
                            value.y = std::max(value.y, bounds.x);
                            value.y = std::min(value.y, bounds.y);
                            value.z = std::max(value.z, bounds.x);
                            value.z = std::min(value.z, bounds.y);
                            value.w = std::max(value.w, bounds.x);
                            value.w = std::min(value.w, bounds.y);
                        }
                        uniform->setValue(value);
                        if (named)
                            shader.setUniformFloat4(uniform->name, value);
                    }
                }
                else
                {
                    if (uniform->gui.showBounds)
                        uniform->gui.showBounds = false;
                    if 
                    (
                        ImGui::ColorEdit4
                        (
                            "##uniformColorEdit4", 
                            glm::value_ptr(value)
                        )
                    )
                    {
                        bounds.x = 0.0;
                        bounds.y = 1.0;
                        uniform->setValue(value);
                        if (named)
                        {
                            shader.setUniformFloat4(uniform->name, value);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                break;
                // Missing Sampler2D/Cubemap, will be added once I implement 
                // the Resource class
            }
            case vir::Shader::Variable::Type::Sampler2D :
            {
                auto resource = 
                    uniform->getValuePtr<Resource>();
                std::string name
                (
                    (resource != nullptr) ? resource->name() : ""
                );
                if (ImGui::BeginCombo("##txSelector", name.c_str()))
                {
                    for(auto r : resources)
                    {
                        if 
                        (
                            r->type() != Resource::Type::Texture2D &&
                            r->type() != Resource::Type::AnimatedTexture2D &&
                            r->type() != Resource::Type::Framebuffer
                        )
                            continue;
                        if (ImGui::Selectable(r->name().c_str()))
                        {
                            uniform->setValuePtr<const Resource>(r);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                    ImGui::EndCombo();
                }
                break;
            }
            case vir::Shader::Variable::Type::SamplerCube :
            {
                auto resource = 
                    uniform->getValuePtr<Resource>();
                std::string name
                (
                    (resource != nullptr) ? resource->name() : ""
                );
                if (ImGui::BeginCombo("##cmSelector", name.c_str()))
                {
                    for(auto r : resources)
                    {
                        if (r->type() != Resource::Type::Cubemap)
                            continue;
                        if (ImGui::Selectable(r->name().c_str()))
                        {
                            uniform->setValuePtr<const Resource>(r);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                    ImGui::EndCombo();
                }
                break;
            }
        }
        END_COLUMN
        
        END_ROW

        if 
        (
            std::find
            (
                uncompiledUniforms.begin(), 
                uncompiledUniforms.end(), 
                uniform
            ) == uncompiledUniforms.end() &&
            (
                uniform->name != name0 ||
                uniform->type != type0
            )
        )
            uncompiledUniforms.emplace_back(uniform);
    }; // End of renderUniforui lambda

    //--------------------------------------------------------------------------
    auto renderAddUniformButton = 
    [&fontSize]
    (
        std::vector<Uniform*>& uniforms, 
        std::vector<Uniform*>& uncompiledUniforms, 
        int& row
    )
    {
        int column;
        START_ROW
        START_COLUMN
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1, 0)))
        {
            auto uniform = new Uniform{};
            uniforms.emplace_back(uniform);
            uncompiledUniforms.emplace_back(uniform);
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Add new uniform");
            ImGui::EndTooltip();
        }
        END_COLUMN
        END_ROW
    }; // End of addNewUniform lambda

    //--------------------------------------------------------------------------
    bool shaderRequiresRecompilation(false);
    static bool showSharedUniforms(true);
    if 
    (
        ImGui::Button
        (
            showSharedUniforms ? 
            "Hide shared uniforms" : 
            "Show shared uniforms",
            {-1,0}
        )
    )
        showSharedUniforms = !showSharedUniforms;
    if 
    (
        ImGui::BeginTable
        (
            "##uniformTable", 
            5, 
            ImGuiTableFlags_BordersV | 
            ImGuiTableFlags_BordersOuterH |
            ImGuiTableFlags_SizingFixedFit
        )
    )
    {
        ImGui::TableSetupColumn("##actions", 0, 4*fontSize);
        ImGui::TableSetupColumn("Name", 0, 10*fontSize);
        ImGui::TableSetupColumn("Type", 0, 5*fontSize);
        ImGui::TableSetupColumn("Bounds", 0, 4*fontSize);
        ImGui::TableSetupColumn
        (
            "Value", 
            0, 
            ImGui::GetContentRegionAvail().x
        );
        ImGui::TableHeadersRow();
        
        if (showSharedUniforms)
            renderSharedUniformsGui(sharedUniforms, row);

        shader.bind(); // Must be bound, else uniforms in renderUniformGui will
                       // not be set
        
        for(auto uniform : uniforms)
        {
            renderUniformGui
            (
                sharedUniforms, // Required to set iUserAction if user changes
                                // anything
                uniform,
                uncompiledUniforms,
                shader,
                resources,
                row
            );
            if (uniform->gui.markedForDeletion)
                shaderRequiresRecompilation = true;
        }
        renderAddUniformButton(uniforms, uncompiledUniforms, row);

        // Remove uniforms marked for deletion
        if (shaderRequiresRecompilation)
        {
            uniforms.erase
            (
                std::remove_if
                (
                    uniforms.begin(),
                    uniforms.end(),
                    [](Uniform* uniform)
                    {
                        if 
                        (
                            uniform->gui.markedForDeletion &&
                            uniform->type == Type::Sampler2D ||
                            uniform->type == Type::SamplerCube
                        )
                        {
                            auto resource = uniform->getValuePtr<Resource>();
                            resource->unbind();
                        }
                        return uniform->gui.markedForDeletion;
                    }
                )
            );
        }

        // Check if all uncompiled uniforms have been named (and thus now
        // eligible for compilation)
        if (uncompiledUniforms.size() > 0)
        {
            bool allUniformsAreNamed = true;
            for (auto* uniform : uncompiledUniforms)
            {
                if (uniform->name.size()>0)
                    continue;
                allUniformsAreNamed = false;
                break;
            }
            if (allUniformsAreNamed)
                shaderRequiresRecompilation = true;
        }

        ImGui::EndTable();
    }
    return shaderRequiresRecompilation;
}

}