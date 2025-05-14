/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthing/include/uniform.h"

#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/shareduniforms.h"
#include "shaderthing/include/statusbar.h"

#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_extensions.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

Uniform::~Uniform()
{
    deleteValue();
}

void Uniform::renderUniformsGui
(
    SharedUniforms& sharedUniforms,
    Layer* layer,
    const std::vector<Layer*>& layers,
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
    [&fontSize]
    (
        Type type, 
        glm::vec2& bounds, 
        float* dragStep = nullptr, 
        float* logarithmicZero = nullptr
    )
    {
        if (ImGui::Button(ICON_FA_RULER_COMBINED, ImVec2(-1, 0)))
            ImGui::OpenPopup("##uniformBounds");
        if (ImGui::BeginPopup("##uniformBounds"))
        {
            
            glm::vec2 bounds0(bounds);
            if (type == vir::Shader::Uniform::Type::UInt)
                bounds.x = std::max(bounds.x, 0.0f);
            ImGui::Text("Minimum value    ");
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
            ImGui::Text("Maximum value    ");
            ImGui::SameLine();
            ImGui::PushItemWidth(inputWidth);
            ImGui::InputFloat
            (
                "##maxValueInput", 
                &(bounds.y), 0.f, 0.f,
                maxf.c_str()
            );
            ImGui::PopItemWidth();
            if 
            (
                (type == Type::Int2 || type == Type::Float2) && 
                dragStep != nullptr
            )
            {
                auto format = Helpers::getFormat(*dragStep);
                ImGui::Text("Mouse drag step  ");
                ImGui::SameLine();
                ImGui::PushItemWidth(inputWidth);
                if (type == Type::Int2)
                {
                    int iDragStep = (int)(*dragStep);
                    ImGui::InputInt
                    (
                        "##dragStepSize", 
                        &iDragStep, 0.f, 0.f
                    );
                    *dragStep = (float)iDragStep;
                }
                else
                    ImGui::InputFloat
                    (
                        "##dragStepSize", 
                        dragStep, 0.f, 0.f,
                        format.c_str()
                    );
                ImGui::PopItemWidth();
            }
            else if 
            (
                logarithmicZero != nullptr &&
                bounds.x * bounds.y <= 0
            )
            {
                auto format = Helpers::getFormat(*logarithmicZero);
                ImGui::Text("Logarithmic zero ");
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(
R"(When a log-scale slider is used and the uniform bounds contain or cross 0, 
this value determines the closest value to 0 (that differs from 0) that can be
set by adjusting the slider)");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::PushItemWidth(inputWidth);
                float logarithmicZero0(*logarithmicZero);
                if 
                (
                    ImGui::InputFloat
                    (
                        "##logarithmicZero", 
                        logarithmicZero, 0.f, 0.f,
                        format.c_str()
                    )
                )
                {
                    if (*logarithmicZero <= 0)
                        *logarithmicZero = logarithmicZero0;
                }
                ImGui::PopItemWidth();
            }
            ImGui::EndPopup();
            return (bounds != bounds0); 
        }
        return false;
    }; // End of renderUniformBoundsButtonGui lambda

    //--------------------------------------------------------------------------
    auto renderDefaultSharedUniformsGui = 
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
            sharedUniforms.toggleRenderingPaused();
            // When stopping rendering while tile rendering is enabled,
            // make sure to render all the tiles to reach the end of the
            // shader frame
            if
            (
                sharedUniforms.flags_.isRenderingPaused && 
                Layer::Rendering::TileController::tiledRenderingEnabled
            )
                sharedUniforms.flags_.stepToNextFrame = true;
        }
        if (sharedUniforms.flags_.isRenderingPaused)
        {
            if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
                sharedUniforms.flags_.stepToNextFrame = true;
            else
            {
                // If tiled rendering is enabled, stepping by one shader frame
                // means stepping by nTiles app frames (since each app frame 
                // only renders a single shader tile), so the frame step is over
                // only once all tiles have been rendered (i.e., when tileIndex
                // is reset to 0)
                if (Layer::Rendering::TileController::tiledRenderingEnabled)
                {
                    if (Layer::Rendering::TileController::tileIndex == 0)
                        sharedUniforms.flags_.stepToNextFrame = false;
                }
                else
                    sharedUniforms.flags_.stepToNextFrame = false;
            }
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Render next frame and increment\niTime by iTimeDelta");
            ImGui::EndTooltip();
        }
        NEXT_COLUMN
        ImGui::Text("iFrame");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::UInt].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text("%d", sharedUniforms.iFrame_);
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
        if (sharedUniforms.flags_.isTimePaused)
        {
            if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
                sharedUniforms.flags_.stepToNextTimeStep = true;
            else 
                sharedUniforms.flags_.stepToNextTimeStep = false;
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Increment iTime by iTimeDelta");
            ImGui::EndTooltip();
        }
        NEXT_COLUMN
        ImGui::Text("iTime");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
        NEXT_COLUMN
        glm::vec2* bounds = &sharedUniforms.iTimeUniform_.gui.bounds;
        bool boundsChanged = renderEditUniformBoundsButtonGui
        (
            Type::Float, 
            sharedUniforms.iTimeUniform_.gui.bounds
        );
        NEXT_COLUMN
        auto iTimePtr = &sharedUniforms.iTime_;
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
        if 
        (
            sharedUniforms.flags_.isRenderingPaused || 
            sharedUniforms.flags_.isTimePaused
        )
        {
            ImGui::InputFloat
            (
                "##iTimeDeltaSliderFloat", 
                &sharedUniforms.iTimeDelta_,
                0,
                0,
                "%.6f"
            );
            sharedUniforms.iTimeDelta_ = 
                std::max(sharedUniforms.iTimeDelta_, 0.f);
            ImGui::SameLine();
            ImGui::Text("s");
        }
        else
            ImGui::Text("%.6f s", sharedUniforms.iTimeDelta_);
        END_ROW
        ImGui::Dummy({0, 0.1f*fontSize});

        // iRandom -------------------------------------------------------------
        START_ROW
        NEXT_COLUMN
        if 
        (
            ImGui::Button
            (
                sharedUniforms.flags_.isRandomNumberGeneratorPaused ? 
                ICON_FA_PLAY : 
                ICON_FA_PAUSE, 
                ImVec2(-1, 0)
            )
        )
            sharedUniforms.flags_.isRandomNumberGeneratorPaused = 
                !sharedUniforms.flags_.isRandomNumberGeneratorPaused;
        NEXT_COLUMN
        ImGui::Text("iRandom");
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
        NEXT_COLUMN
        // No bounds
        NEXT_COLUMN
        ImGui::Text("%.6f", sharedUniforms.iRandom_);
        END_ROW

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
            "%.3f", sharedUniforms.iAspectRatio_
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
            (int)sharedUniforms.iResolution_.x, 
            (int)sharedUniforms.iResolution_.y
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
            auto& keyData(sharedUniforms.iKeyboard_[key]);
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
        /*
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
        */
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
            ImGui::OpenPopup("##iMouseSettings");
        if (ImGui::BeginPopup("##iMouseSettings"))
        {
            bool enabled = sharedUniforms.flags_.isMouseInputEnabled;
            std::string text = enabled ? "Disable inputs" : "Enable inputs";
            if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
                sharedUniforms.toggleMouseInputs();
            ImGui::Text("Clamp value to window resolution ");
            ImGui::SameLine();
            bool status = sharedUniforms.flags_.isMouseInputClampedToWindow;
            ImGui::Checkbox("##iMouseSettings_ClampValue", &status);
            if (status != sharedUniforms.flags_.isMouseInputClampedToWindow)
                sharedUniforms.setMouseInputsClamped(status);
            ImGui::Text("Input requires holding LMB       ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(If true, the iMouse uniform value will change on mouse 
motion only if the left mouse button (LMB) is held)");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::Checkbox
            (
                "##iMouseSettings_LMBHold", 
                &(sharedUniforms.flags_.mouseInputRequiresLMBHold)
            );
            ImGui::EndPopup();
        }
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
            (int)sharedUniforms.iMouse_.x, 
            (int)sharedUniforms.iMouse_.y, 
            (int)sharedUniforms.iMouse_.z, 
            (int)sharedUniforms.iMouse_.w
        );
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
R"(The first two components (x, y) are the current x, y coordinates (with respect
to the lower-left corner of the ShaderThing window) of the mouse cursor if the
left mouse button is currently being held down. The last two components (z, w)
represent the x, y coordinates of the last left mouse button click, with their 
sign reversed. If the sign of the z component is positive, then the left mouse
is currently being held down)");
            ImGui::EndTooltip();
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
            std::string text = enabled ? "Disable inputs" : "Enable inputs";
            if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
                sharedUniforms.toggleCameraMouseInputs();
            ImGui::Text("Input requires holding LMB       ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(If true, the iLook uniform value will change on mouse 
motion only if the left mouse button (LMB) is held)");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::Checkbox
            (
                "##iLookSettings_LMBHold", 
                &(sharedUniforms.flags_.cameraMouseInputRequiresLMBHold)
            );
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
            glm::vec3 value = sharedUniforms.iLook_;
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
                sharedUniforms.iLook_ = value;
                sharedUniforms.shaderCamera_->setDirection(value);
                sharedUniforms.setUserAction(true);
            }
            ImGui::PopItemWidth();
        }
        END_ROW

        // iWASD ---------------------------------------------------------------
        bool showSeparator(sharedUniforms.userUniforms_.size() == 0);
        float posY = 0;
        START_ROW
        NEXT_COLUMN
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
            ImGui::OpenPopup("##iWASDSettings");
        if (ImGui::BeginPopup("##iWASDSettings"))
        {
            bool enabled = sharedUniforms.flags_.isCameraKeyboardInputEnabled;
            std::string text = enabled ? "Disable inputs" : "Enable inputs";
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
        if (showSeparator)
        {
            posY = ImGui::GetCursorPosY();
            ImGui::Separator();
        }
        NEXT_COLUMN
        ImGui::Text("iWASD");
        if (showSeparator)
        {
            ImGui::SetCursorPosY(posY);
            ImGui::Separator();
        }
        NEXT_COLUMN
        ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
        if (showSeparator)
        {
            ImGui::SetCursorPosY(posY);
            ImGui::Separator();
        }
        NEXT_COLUMN
        bounds = &sharedUniforms.iWASDUniform_.gui.bounds;
        boundsChanged = renderEditUniformBoundsButtonGui
        (
            Type::Float3, 
            sharedUniforms.iWASDUniform_.gui.bounds
        );
        if (showSeparator)
            ImGui::Separator();
        NEXT_COLUMN
        {
            glm::vec3 value = sharedUniforms.iWASD_;
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
                sharedUniforms.iWASD_ = value;
                sharedUniforms.shaderCamera_->setPosition(value);
                sharedUniforms.setUserAction(true);
            }
            ImGui::PopItemWidth();
        }
        if (showSeparator)
            ImGui::Separator();
        END_ROW

        //ImGui::Dummy({0, 0.05f*fontSize});
    }; // End of renderSharedUniformsGui lambda

    // -------------------------------------------------------------------------
    static std::string supportedUniformTypeNames[15]
    {
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Bool],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Int],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Int2],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Int3],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Int4],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Float],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Float2],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Float3],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Float4],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Sampler2D],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Sampler3D],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::SamplerCube],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Image2D],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::Image3D],
        vir::Shader::uniformTypeToName[vir::Shader::Uniform::Type::ImageCube]
    };
    auto renderUniformGui = 
    [&fontSize, &renderEditUniformBoundsButtonGui]
    (
        SharedUniforms& sharedUniforms,
        Uniform* uniform,
        Layer* layer,
        const std::vector<Layer*>& layers,
        const std::vector<Resource*>& resources,
        int& row,
        const bool showSeparator = false,
        const bool showSharedAndDefaultUniforms = true
    ) -> bool
    {
        int column;
        bool managed
        (
            uniform->specialType == SpecialType::LayerAspectRatio ||
            uniform->specialType == SpecialType::LayerResolution
        );
        if (managed && !showSharedAndDefaultUniforms)
            return false;
        bool isSharedByUser0 = uniform->isSharedByUser;
        bool nameChanged = false;
        bool typeChanged = false;
        
        START_ROW

        START_COLUMN // Action column ------------------------------------------
        float y0 = 0;
        if (!managed)
        {
            float halfButtonSize(1.7*fontSize);
            if (ImGui::Button(ICON_FA_TRASH, ImVec2(halfButtonSize, 0)))
            {
                uniform->gui.markedForDeletion = true;
                layer->uniformBuffer_->removeUniform(uniform);
                // The uniform is gonna get deleted, so the layer(s) using it
                // will have to be recompiled
                if (uniform->isSharedByUser)
                {
                    for (auto l : layers)
                        l->flags_.uncompiledChanges = true;
                }
                else
                    layer->flags_.uncompiledChanges = true;
            }
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text("Delete this uniform");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            if 
            (
                ImGui::Button
                (
                    !uniform->isSharedByUser ?
                    ICON_FA_ARROW_UP :
                    ICON_FA_ARROW_DOWN,
                    {-1, 0}
                )
            )
            {
                uniform->hasSharedByUserChanged = true;
                uniform->isSharedByUser = 
                    !uniform->isSharedByUser;
            }
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
            {
                if (!uniform->isSharedByUser)
                    ImGui::Text("Share this uniform across all layers");
                else
                    ImGui::Text(
"Remove this uniform from shared\nuniforms across all layers"\
                    );
                ImGui::EndTooltip();
            }
            y0 = ImGui::GetCursorPosY();
        }
        if (showSeparator)
            ImGui::Separator();
        END_COLUMN
        
        START_COLUMN // Name column --------------------------------------------
        if (managed)
            ImGui::Text(uniform->name.c_str());
        else
        {
            if (ImGui::InputText("##uniformName", &uniform->name))
            {
                if (!uniform->isSharedByUser)
                    layer->uniformBuffer_->markUniformForSubmission(uniform);
                else
                    sharedUniforms.fBuffer_->markUniformForSubmission(uniform);
                nameChanged = true;
            }
        }
        bool named(uniform->name.size() > 0);
        if (showSeparator)
            ImGui::Separator();
        END_COLUMN

        START_COLUMN // Type column --------------------------------------------
        if (managed)
            ImGui::Text(vir::Shader::uniformTypeToName[uniform->type()].c_str());
        else if 
        (
            ImGui::BeginCombo
            (
                "##uniformTypeSelector", 
                vir::Shader::uniformTypeToName[uniform->type()].c_str()
            )
        )
        {
            for(auto uniformTypeName : supportedUniformTypeNames)
            {
                if (!ImGui::Selectable(uniformTypeName.c_str()))
                    continue;
                auto selectedType = 
                    vir::Shader::uniformNameToType[uniformTypeName];
                if (selectedType == uniform->type())
                    continue;
                typeChanged = true;
                bool typeIsSamplerOrImage2D = 
                (
                    uniform->type() == 
                    vir::Shader::Uniform::Type::Sampler2D ||
                    uniform->type() == 
                    vir::Shader::Uniform::Type::Image2D
                );
                bool selectedTypeIsSamplerOrImage2D = 
                (
                    selectedType == 
                    vir::Shader::Uniform::Type::Sampler2D ||
                    selectedType == 
                    vir::Shader::Uniform::Type::Image2D
                );
                bool typeIsSamplerOrImage3D = 
                (
                    uniform->type() == 
                    vir::Shader::Uniform::Type::Sampler3D ||
                    uniform->type() == 
                    vir::Shader::Uniform::Type::Image3D
                );
                bool selectedTypeIsSamplerOrImage3D = 
                (
                    selectedType == 
                    vir::Shader::Uniform::Type::Sampler3D ||
                    selectedType == 
                    vir::Shader::Uniform::Type::Image3D
                );
                bool typeIsSamplerOrImageCube = 
                (
                    uniform->type() == 
                    vir::Shader::Uniform::Type::SamplerCube ||
                    uniform->type() == 
                    vir::Shader::Uniform::Type::ImageCube
                );
                bool selectedTypeIsSamplerOrImageCube = 
                (
                    selectedType == 
                    vir::Shader::Uniform::Type::SamplerCube ||
                    selectedType == 
                    vir::Shader::Uniform::Type::ImageCube
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
                    auto resource = uniform->getValuePtr<Resource>();
                    if (resource != nullptr)
                        resource->removeClientUniform(uniform);
                }
                else if (typeChangedFromNonResourceToResource)
                    layer->uniformBuffer_->removeUniform(uniform);

                if 
                (
                    typeChangedFromResourceToNonResourceType ||
                    typeChangedFromResourceToIncompatibleResource
                )
                    layer->uniformBuffer_->removeUniform
                    (
                        uniform->resourceResolution_.get()
                    );
                
                uniform->setType(selectedType, true);
                uniform->gui.showBounds = 
                (
                    selectedType != vir::Shader::Uniform::Type::Bool &&
                    selectedType != vir::Shader::Uniform::Type::Sampler2D &&
                    selectedType != vir::Shader::Uniform::Type::Sampler3D &&
                    selectedType != vir::Shader::Uniform::Type::SamplerCube &&
                    selectedType != vir::Shader::Uniform::Type::Image2D &&
                    selectedType != vir::Shader::Uniform::Type::Image3D &&
                    selectedType != vir::Shader::Uniform::Type::ImageCube
                );

                // Add uniform to the buffer if it is a non-resource type now
                // that the type has been set (cannot do it before, as I need
                // to have the new uniform type already set before adding the
                // uniform the buffer)
                if (typeChangedFromResourceToNonResourceType)
                    layer->uniformBuffer_->addUniform(uniform);

                if 
                (
                        selectedTypeIsSamplerOrImage2D ||
                        selectedTypeIsSamplerOrImage3D ||
                        selectedTypeIsSamplerOrImageCube
                )
                    continue;
                
                layer->uniformBuffer_->
                    recalculateUniformSizesAndOffsets();
                layer->uniformBuffer_->
                    markUniformForSubmission(uniform);
            }
            ImGui::EndCombo();
        }
        if (showSeparator)
            ImGui::Separator();
        END_COLUMN

        START_COLUMN // Bounds column ------------------------------------------
        bool boundsChanged(false);
        glm::vec2& bounds = uniform->gui.bounds;
        if (uniform->gui.showBounds)
        {
            boundsChanged = renderEditUniformBoundsButtonGui
            (
                uniform->type(),
                bounds,
                &(uniform->gui.dragStep),
                uniform->isLogarithmic ? 
                    &(uniform->gui.logarithmicZero) : nullptr
            );
        }
        if (showSeparator)
        {
            if (y0 > 0)
                ImGui::SetCursorPosY(y0);
            ImGui::Separator();
        }
        END_COLUMN

#define SET_UNIFORM_VALUE(Type)                                             \
    if (!isSharedByUser0)                                                   \
    {                                                                       \
        layer->uniformBuffer_->markUniformForSubmission(uniform);           \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        sharedUniforms.fBuffer_->markUniformForSubmission(uniform);         \
    }

#define CHECK_RESOURCE_SELECTED                                             \
    if (ImGui::Selectable(r->name().c_str()))                               \
    {                                                                       \
        if (resource != nullptr)                                            \
        {                                                                   \
            Layer::Flags::requestRecompilation =                            \
                Layer::Flags::requestRecompilation ||                       \
                resource->isInternalFormatUnsigned() !=                     \
                r->isInternalFormatUnsigned();                              \
            if (resource->isUsedByUniform(uniform))                         \
                resource->removeClientUniform(uniform);                     \
        }                                                                   \
        else                                                                \
            Layer::Flags::requestRecompilation = true;                      \
        if (!r->isUsedByUniform(uniform))                                   \
            r->addClientUniform(uniform);                                   \
        auto ubo = uniform->isSharedByUser ?                                \
            sharedUniforms.fBuffer_.get() : layer->uniformBuffer_.get();    \
        uniform->setResourcePtr(r, ubo);                                    \
        sharedUniforms.setUserAction(true);                                 \
    }

        START_COLUMN // Value column -------------------------------------------
        switch(uniform->type())
        {
            case vir::Shader::Uniform::Type::Bool :
            {
                auto value = uniform->getValue<bool>();
                if (ImGui::Checkbox((value) ? "true" : "false", &value))
                {
                    uniform->setValue(value, Type::Bool);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Bool)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::UInt : //-------------------------
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
                    uniform->setValue(value, Type::UInt);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Int)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Int : //--------------------------
            {
                auto value = uniform->getValue<int>();
                if (!boundsChanged)
                {
                    bounds.x = std::min((float)value, bounds.x);
                    bounds.y = std::max((float)value, bounds.y);
                }
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
                    uniform->setValue(value, Type::Int);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Int)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Int2 : //-------------------------
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
                        auto valuei0 = uniform->getCache<glm::ivec2>();
                        value.x = valuei0.x + 
                            (float)(10.f*delta.x*uniform->gui.dragStep)/maxRes;
                        value.y = valuei0.y + 
                            (float)(10.f*delta.y*uniform->gui.dragStep)/maxRes;
                        input = true;
                    }
                    else
                        uniform->setCache<glm::ivec2>(value);
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
                    uniform->setValue(value, Type::Int2);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Int2)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Int3 : //-------------------------
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
                    uniform->setValue(value, Type::Int3);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Int3)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Int4 : //-------------------------
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
                    uniform->setValue(value, Type::Int4);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Int4)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Float : //------------------------
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
                    if (ImGui::SmallButton(uniform->isLogarithmic?"log":"lin"))
                        uniform->isLogarithmic = !uniform->isLogarithmic;
                    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                    {
                        ImGui::Text
                        ( 
                            uniform->isLogarithmic ?
                            "Switch slider to linear scale" : 
                            "Switch slider to logarithmic scale"
                        );
                        ImGui::EndTooltip();
                    }
                    ImGui::SameLine();
                    ImGuiSliderFlags flags = 0;
                    std::string format;
                    if (uniform->isLogarithmic)
                    {
                        ImGui::PushDragSliderLogZeroForScientificNotation
                        (
                            uniform->gui.logarithmicZero
                        );
                        format = "%.3e";
                        flags = ImGuiSliderFlags_Logarithmic;
                    } 
                    else
                        format = Helpers::getFormat(value);
                    input = ImGui::SliderFloat
                    (
                        "##fSlider", 
                        &value, 
                        bounds.x,
                        bounds.y,
                        format.c_str(),
                        flags
                    );
                    if (uniform->isLogarithmic)
                        ImGui::PopDragSliderLogZeroForScientificNotation();
                }
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value = std::max(value, bounds.x);
                        value = std::min(value, bounds.y);
                    }
                    uniform->setValue(value, Type::Float);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Float)                                                                
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Float2 : //-----------------------
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
                    auto value = uniform->getValue<glm::vec2>();
                    ImGui::Text
                    (
                        "%d x %d",
                        (int)value.x, (int)value.y
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
                        auto valuef0 = uniform->getCache<glm::vec2>();
                        value.x = valuef0.x + 
                            (float)(10.f*delta.x*uniform->gui.dragStep)/maxRes;
                        value.y = valuef0.y + 
                            (float)(10.f*delta.y*uniform->gui.dragStep)/maxRes;
                        input = true;
                    }
                    else
                        uniform->setCache<glm::vec2>(value);
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
                    uniform->setValue(value, Type::Float2);
                    if (named)
                    {
                        SET_UNIFORM_VALUE(Float2)
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Float3 : //-----------------------
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
                        uniform->setValue(value, Type::Float3);
                        if (named)
                        {
                            SET_UNIFORM_VALUE(Float3)
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
                        uniform->setValue(value, Type::Float3);
                        if (named)
                        {
                            SET_UNIFORM_VALUE(Float3);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Float4 : //-----------------------
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
                        uniform->setValue(value, Type::Float4);
                        if (named)
                        {
                            SET_UNIFORM_VALUE(Float4)
                        }
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
                        uniform->setValue(value, Type::Float4);
                        if (named)
                        {
                            SET_UNIFORM_VALUE(Float4)
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                break;
            }
            case vir::Shader::Uniform::Type::Sampler2D :
            case vir::Shader::Uniform::Type::Image2D :
            {
                auto resource = 
                    uniform->getValuePtr<Resource>();
                std::string name
                (
                    (resource != nullptr) ? resource->name() : ""
                );
                if (ImGui::BeginCombo("##tx2DSelector", name.c_str()))
                {
                    for (auto r : resources)
                    {
                        if 
                        (
                            r->type() != Resource::Type::Texture2D &&
                            r->type() != Resource::Type::AnimatedTexture2D &&
                            r->type() != Resource::Type::Framebuffer
                        )
                            continue;
                        CHECK_RESOURCE_SELECTED
                    }
                    ImGui::EndCombo();
                }
                break;
            }
            case vir::Shader::Uniform::Type::Sampler3D :
            case vir::Shader::Uniform::Type::Image3D :
            {
                auto resource = 
                    uniform->getValuePtr<Resource>();
                std::string name
                (
                    (resource != nullptr) ? resource->name() : ""
                );
                if (ImGui::BeginCombo("##tx3DSelector", name.c_str()))
                {
                    for (auto r : resources)
                    {
                        if (r->type() != Resource::Type::Texture3D)
                            continue;
                        CHECK_RESOURCE_SELECTED
                    }
                    ImGui::EndCombo();
                }
                break;
            }
            case vir::Shader::Uniform::Type::SamplerCube :
            case vir::Shader::Uniform::Type::ImageCube :
            {
                auto resource = 
                    uniform->getValuePtr<Resource>();
                std::string name
                (
                    (resource != nullptr) ? resource->name() : ""
                );
                if (ImGui::BeginCombo("##cmSelector", name.c_str()))
                {
                    for (auto r : resources)
                    {
                        if (r->type() != Resource::Type::Cubemap)
                            continue;
                        CHECK_RESOURCE_SELECTED
                    }
                    ImGui::EndCombo();
                }
                break;
            }
            default:
                break;
        }
        if (showSeparator)
            ImGui::Separator();
        END_COLUMN
        
        END_ROW

        if (!nameChanged && !typeChanged)
            return false;

        // If the uniform name or type have changed, it should be added to the
        // list of uncompiled uniforms of the layer using this uniform (if this
        // uniform is not SharedByUser) or of all the layers (if this uniform
        // is SharedByUser). Also, if at least one uniform has been named,
        // said layer should be recompiled (all other layer uniforms which are
        // still unnamed are simply ignored)
        if (isSharedByUser0)
        {
            for (auto l : layers)
            {
                if 
                (
                    std::find // I.e., if not already in uncompiledUniforms
                    (
                        l->cache_.uncompiledUniforms.begin(), 
                        l->cache_.uncompiledUniforms.end(), 
                        uniform
                    ) == l->cache_.uncompiledUniforms.end()
                )
                    l->cache_.uncompiledUniforms.emplace_back(uniform);
                bool atLeastOneUniformNamed = false;
                for (auto* u : l->cache_.uncompiledUniforms)
                {
                    if (u->name.size() == 0)
                        continue;
                    atLeastOneUniformNamed = true;
                    break;
                }
                if (atLeastOneUniformNamed)
                    l->flags_.uncompiledChanges = true;
            }
        }
        else
        {
            if 
            (
                std::find
                (
                    layer->cache_.uncompiledUniforms.begin(), 
                    layer->cache_.uncompiledUniforms.end(), 
                    uniform
                ) == layer->cache_.uncompiledUniforms.end()
            )
                layer->cache_.uncompiledUniforms.emplace_back(uniform);
            bool atLeastOneUniformNamed = false;
            for (auto* u : layer->cache_.uncompiledUniforms)
            {
                if (u->name.size() == 0)
                    continue;
                atLeastOneUniformNamed = true;
                break;
            }
            if (atLeastOneUniformNamed)
                layer->flags_.uncompiledChanges = true;
        }

        return typeChanged;
        
    }; // End of renderUniform lambda

    //--------------------------------------------------------------------------
    auto renderAddUniformButton = 
    [&fontSize]
    (
        Layer* layer,
        int& row
    )
    {
        int column;
        START_ROW
        START_COLUMN
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1, 0)))
            layer->addUniform(new Uniform{});
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
    bool atLeastOneUniformMarkedForDeletion = false;
    bool atLeastOneSharedUniformTypeChanged = false;
    bool atLeastOneUniformTypeChanged = false;

    bool atLeastOneSharedUniformStateChanged = false;
    static bool showSharedAndDefaultUniforms = true;
    if 
    (
        ImGui::Button
        (
            showSharedAndDefaultUniforms ? 
            "Hide shared and default uniforms" : 
            "Show shared and uniforms",
            {-1,0}
        )
    )
        showSharedAndDefaultUniforms = !showSharedAndDefaultUniforms;

    std::string uniformFrameName = "##uniformsFrame"+std::to_string(layer->id_);
    ImGui::BeginChild(uniformFrameName.c_str(), ImVec2(-1, -1), false);

    int nColumns = 5;
    if 
    (
        ImGui::BeginTable
        (
            "##uniformTable", 
            nColumns, 
            ImGuiTableFlags_BordersV | 
            ImGuiTableFlags_BordersOuterH |
            ImGuiTableFlags_SizingFixedFit
        )
    )
    {
        ImGui::TableSetupColumn("##actions", 0, 4*fontSize);
        ImGui::TableSetupColumn("Name", 0, 10*fontSize);
        ImGui::TableSetupColumn("Type", 0, 7*fontSize);
        ImGui::TableSetupColumn("Bounds", 0, 3.5*fontSize);
        ImGui::TableSetupColumn
        (
            "Value", 
            0, 
            ImGui::GetContentRegionAvail().x
        );
        ImGui::TableHeadersRow();
        
        if (showSharedAndDefaultUniforms)
            renderDefaultSharedUniformsGui(sharedUniforms, row);

        for (auto uniform : sharedUniforms.userUniforms_)
        {
            atLeastOneSharedUniformStateChanged = 
                atLeastOneSharedUniformStateChanged ||
                renderUniformGui
                (
                    sharedUniforms,
                    uniform,
                    layer,
                    layers,
                    resources,
                    row,
                    uniform == sharedUniforms.userUniforms_.back()
                );
            if (uniform->gui.markedForDeletion)
                atLeastOneUniformMarkedForDeletion = true;
            if (uniform->hasSharedByUserChanged)
                atLeastOneSharedUniformStateChanged = true;
        }

        for(auto uniform : layer->uniforms_)
        {
            atLeastOneUniformTypeChanged = 
                atLeastOneUniformTypeChanged ||
                renderUniformGui
                (
                    sharedUniforms,
                    uniform,
                    layer,
                    layers,
                    resources,
                    row,
                    false,
                    showSharedAndDefaultUniforms
                );
            if (uniform->gui.markedForDeletion)
                atLeastOneUniformMarkedForDeletion = true;
            if (uniform->hasSharedByUserChanged)
                atLeastOneSharedUniformStateChanged = true;
        }
        renderAddUniformButton
        (
            layer, 
            row
        );
        ImGui::EndTable();
    }

    ImGui::EndChild();

    ImGui::SetCursorPosY
    (
        ImGui::GetCursorPosY()+
        ImGui::GetContentRegionAvail().y-
        ImGui::GetTextLineHeightWithSpacing()
    );
    StatusBar::renderGui();

    // Remove uniforms marked for deletion
    if (atLeastOneUniformMarkedForDeletion)
    {
        for (unsigned int i=0; i<layer->uniforms_.size(); i++)
        {
            auto u = layer->uniforms_[i];
            if (!u->gui.markedForDeletion)
                continue;
            if (u->isResource())
            {
                auto resource = u->getValuePtr<Resource>();
                if (resource->isUsedByUniform(u))
                    resource->removeClientUniform(u);
                resource->unbind();
            }
            layer->removeUniform(u);
            delete u;
            i--;
        }
        for (unsigned int i=0; i<sharedUniforms.userUniforms_.size(); i++)
        {
            auto u = sharedUniforms.userUniforms_[i];
            if (!u->gui.markedForDeletion)
                continue;
            if (u->isResource())
            {
                auto resource = u->getValuePtr<Resource>();
                if (resource->isUsedByUniform(u))
                    resource->removeClientUniform(u);
                resource->unbind();
            }
            sharedUniforms.removeUserUniform(u);
            delete u;
            i--;
            atLeastOneSharedUniformStateChanged = true;
        }
        /*
        layer->uniforms_.erase
        (
            std::remove_if
            (
                layer->uniforms_.begin(),
                layer->uniforms_.end(),
                [](Uniform* uniform)
                {
                    if 
                    (
                        uniform->gui.markedForDeletion &&
                        (
                            uniform->type() == Type::Sampler2D ||
                            uniform->type() == Type::Sampler3D ||
                            uniform->type() == Type::SamplerCube ||
                            uniform->type() == Type::Image2D ||
                            uniform->type() == Type::Image3D ||
                            uniform->type() == Type::ImageCube
                        )
                    )
                    {
                        auto resource = uniform->getValuePtr<Resource>();
                        if (resource->isUsedByUniform(uniform))
                            resource->removeClientUniform(uniform);
                        resource->unbind();
                    }
                    return uniform->gui.markedForDeletion;
                }
            )
        );
        // The uncompiledChanges flag of the relevant layers have already
        // been set earlier, where the uniform markedForDeletion flag is set
        */
    }

    // Alternative strategy to cope with uniform block alignment changes after
    // uniform type changes or deletions (both of which can alter block layout:
    // compile right away automatically without asking the user
    if 
    (
        atLeastOneUniformTypeChanged ||
        atLeastOneUniformMarkedForDeletion
    )
        layer->compileShader(sharedUniforms);

    if (!atLeastOneSharedUniformStateChanged)
        return;

    // Check if the uniform state was changed from non-shared to shared
    for (auto uniform : layer->uniforms_)
    {
        if (!(uniform->hasSharedByUserChanged && uniform->isSharedByUser))
            continue;
        layer->removeUniform(uniform);
        sharedUniforms.addUserUniform(uniform);
        uniform->hasSharedByUserChanged = false;
    }

    // Check if the uniform state was changed from shared to non-shared
    for (auto uniform : sharedUniforms.userUniforms_)
    {
        if (!(uniform->hasSharedByUserChanged && !uniform->isSharedByUser))
            continue;
        sharedUniforms.removeUserUniform(uniform);
        layer->addUniform(uniform);
        uniform->hasSharedByUserChanged = false;
    }

    // Alternative strategy to cope with uniform block alignment changes after
    // uniform type changes or deletions (both of which can alter block layout:
    // compile right away automatically without asking the user
    for (auto* l : layers)
    {
        l->compileShader(sharedUniforms);
    }
}

void Uniform::loadAll
(
    const ObjectIO& io, 
    std::vector<Uniform*>& uniforms,
    vir::DynamicUniformBuffer* uniformBuffer,
    const std::vector<Resource*>& resources,
    std::map<Uniform*, std::string>& uninitializedResourceLayers
)
{
    if (!io.hasMember("uniforms"))
        return;
    auto uniformsData = io.readObject("uniforms");
    for(auto uniformName : uniformsData.members())
    {
        auto uniformData = uniformsData.readObject(uniformName);
        auto uniform = new Uniform{};
        // Mapping for compatibility with previous version .stf files
        std::string typeName = uniformData.read<std::string>("type");
        if (typeName == "texture2D")
            typeName = "sampler2D";
        else if (typeName == "cubemap")
            typeName = "samplerCube";
        auto type = vir::Shader::uniformNameToType[typeName];
        uniform->isSharedByUser = 
            uniformData.readOrDefault<bool>("shared", false);
        uniforms.emplace_back(uniform);
        float min = 0., max = 0.;

#define SET_UNIFORM(type, uType)                                             \
        uniform->setValue<type>(uniformData.read<type>("value"), Type::uType);
#define READ_MIN_MAX                                                         \
        min = uniformData.read<float>("min");                                \
        max = uniformData.read<float>("max");
        bool setInUniformBuffer = true;
        switch (type)
        {
            case vir::Shader::Uniform::Type::Bool :
            {
                SET_UNIFORM(bool, Bool)
                uniform->gui.showBounds = false;
                break;
            }
            case vir::Shader::Uniform::Type::UInt :
            {
                SET_UNIFORM(unsigned int, UInt)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Int :
            {
                SET_UNIFORM(int, Int)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Int2 :
            {
                SET_UNIFORM(glm::ivec2, Int2)
                READ_MIN_MAX
                uniform->gui.dragStep = 
                    uniformData.readOrDefault<float>("dragStep", 1.f);
                break;
            }
            case vir::Shader::Uniform::Type::Int3 :
            {
                SET_UNIFORM(glm::ivec3, Int3)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Int4 :
            {
                SET_UNIFORM(glm::ivec4, Int4)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Float :
            {
                SET_UNIFORM(float, Float)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Float2 :
            {
                SET_UNIFORM(glm::vec2, Float2)
                READ_MIN_MAX
                uniform->gui.dragStep = 
                    uniformData.readOrDefault<float>("dragStep", 1.f);
                break;
            }
            case vir::Shader::Uniform::Type::Float3 :
            {
                SET_UNIFORM(glm::vec3, Float3)
                READ_MIN_MAX
                uniform->gui.usesColorPicker = uniformData.read<bool>(
                    "usesColorPicker");
                uniform->gui.showBounds = !uniform->gui.usesColorPicker;
                break;
            }
            case vir::Shader::Uniform::Type::Float4 :
            {
                SET_UNIFORM(glm::vec4, Float4)
                READ_MIN_MAX
                uniform->gui.usesColorPicker = uniformData.read<bool>(
                    "usesColorPicker");
                uniform->gui.showBounds = !uniform->gui.usesColorPicker;
                break;
            }
            case vir::Shader::Uniform::Type::Sampler2D :
            case vir::Shader::Uniform::Type::Sampler3D :
            case vir::Shader::Uniform::Type::SamplerCube :
            case vir::Shader::Uniform::Type::Image2D :
            case vir::Shader::Uniform::Type::Image3D :
            case vir::Shader::Uniform::Type::ImageCube :
            {
                uniform->setType(type);
                uniform->gui.showBounds = false;
                std::string resourceName = uniformData.read("value", false);
                bool found = false;
                for (auto resource : resources)
                {
                    if (resource->name() == resourceName)
                    {
                        uniform->setResourcePtr(resource, uniformBuffer);
                        found = true;
                        break;
                    }
                }
                if (!found)
                    uninitializedResourceLayers.insert
                    (
                        {uniform, resourceName}
                    );
                setInUniformBuffer = false;
                break;
            }
            default:
                break;
        }
        uniform->gui.bounds = {min, max};
        uniform->name = uniformName;
        if (setInUniformBuffer && uniformBuffer != nullptr)
            uniformBuffer->addUniform(uniform);
    }
}

void Uniform::deleteValue(bool deleteCache)
{
    vir::Shader::Uniform::deleteValue(deleteCache);
    //DELETE_IF_NOT_NULLPTR(resourceResolution_)
    resourceResolution_.reset();
}

bool Uniform::isResource() const 
{
    switch(type())
    {
        case vir::Shader::Uniform::Type::Sampler2D :
        case vir::Shader::Uniform::Type::Image2D :
        case vir::Shader::Uniform::Type::SamplerCube :
        case vir::Shader::Uniform::Type::ImageCube :
        case vir::Shader::Uniform::Type::Sampler3D :
        case vir::Shader::Uniform::Type::Image3D :
            return true;
        default :
            return false;
    }
}

void Uniform::setResourcePtr
(
    Resource* resource, 
    vir::DynamicUniformBuffer* uniformBuffer
)
{
    bool is3D;
    switch(type())
    {
        case vir::Shader::Uniform::Type::Sampler2D :
        case vir::Shader::Uniform::Type::Image2D :
        case vir::Shader::Uniform::Type::SamplerCube :
        case vir::Shader::Uniform::Type::ImageCube :
            is3D = false;
            break;
        case vir::Shader::Uniform::Type::Sampler3D :
        case vir::Shader::Uniform::Type::Image3D :
            is3D = true;
            break;
        default :
            return;
    }

    vir::Shader::Uniform::setValuePtr(resource, type(), false);
    if (resource == nullptr)
        return;

    // Also set resolution uniform
    if (resourceResolution_ == nullptr)
        resourceResolution_ = vir::makeUnique<Uniform>();
    resourceResolution_->name = name+"Resolution";
    if (is3D)
        resourceResolution_->setValue
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
        resourceResolution_->setValue
        (
            glm::vec2
            (
                resource->width(), 
                resource->height()
            ),
            Type::Float2
        );
    if (uniformBuffer != nullptr)
        uniformBuffer->addUniform(resourceResolution_.get());
}

void Uniform::removeResourceResolutionFromUniformBuffer
(
    vir::DynamicUniformBuffer* uniformBuffer
)
{
    if (resourceResolution_ == nullptr)
        return;
    uniformBuffer->removeUniform(resourceResolution_.get());
}

void Uniform::updateResourceResolution
(
    vir::DynamicUniformBuffer* uniformBuffer
)
{
    if (resourceResolution_ == nullptr)
        return;
    switch(type())
    {
        case vir::Shader::Uniform::Type::Sampler2D :
        case vir::Shader::Uniform::Type::Image2D :
        case vir::Shader::Uniform::Type::SamplerCube :
        case vir::Shader::Uniform::Type::ImageCube :
        case vir::Shader::Uniform::Type::Sampler3D :
        case vir::Shader::Uniform::Type::Image3D :
            break;
        default :
            return;
    }
    auto* resource = getValuePtr<Resource>();
    if (resource == nullptr)
        return;
    if (resourceResolution_->type() == Uniform::Type::Float2)
    {
        auto* value = resourceResolution_->getValuePtr<glm::vec2>();
        if 
        (
            value->x != resource->width() || 
            value->y != resource->height()
        )
        {
            value->x = resource->width();
            value->y = resource->height();
            uniformBuffer->markUniformForSubmission
            (
                resourceResolution_.get()
            );
        }
    }
    else if (resourceResolution_->type() == Uniform::Type::Float3)
    {
        auto* value = resourceResolution_->getValuePtr<glm::vec3>();
        if 
        (
            value->x != resource->width() || 
            value->y != resource->height() ||
            value->z != resource->depth()
        )
        {
            value->x = resource->width();
            value->y = resource->height();
            value->z = resource->depth();
            uniformBuffer->markUniformForSubmission
            (
                resourceResolution_.get()
            );
        }
    }
}

void Uniform::updateResourceResolutionName()
{
    if (resourceResolution_ != nullptr)
        resourceResolution_->name = name+"Resolution";
}

void Uniform::saveAll(ObjectIO& io, const std::vector<Uniform*>& uniforms)
{
    io.writeObjectStart("uniforms");
    for (auto u : uniforms)
    {
        float& min(u->gui.bounds.x);
        float& max(u->gui.bounds.y);
        if 
        (
            u->name.size() == 0 || 
            u->specialType != Uniform::SpecialType::None
        )
            continue;
        io.writeObjectStart(u->name.c_str());
        io.write("type", vir::Shader::uniformTypeToName[u->type()].c_str());
        io.write("shared", u->isSharedByUser);

#define WRITE_MIN_MAX           \
        io.write("min", min);   \
        io.write("max", max);

        switch(u->type())
        {
            case vir::Shader::Uniform::Type::Bool :
            {
                io.write("value", u->getValue<bool>());
                break;
            }
            case vir::Shader::Uniform::Type::Int :
            {
                io.write("value", u->getValue<int>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Int2 :
            {
                io.write("value", u->getValue<glm::ivec2>());
                WRITE_MIN_MAX
                io.write("dragStep", u->gui.dragStep);
                break;
            }
            case vir::Shader::Uniform::Type::Int3 :
            {
                io.write("value", u->getValue<glm::ivec3>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Int4 :
            {
                io.write("value", u->getValue<glm::ivec4>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Float :
            {
                io.write("value", u->getValue<float>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Uniform::Type::Float2 :
            {
                io.write("value", u->getValue<glm::vec2>());
                WRITE_MIN_MAX
                io.write("dragStep", u->gui.dragStep);
                break;
            }
            case vir::Shader::Uniform::Type::Float3 :
            {
                io.write("value", u->getValue<glm::vec3>());
                WRITE_MIN_MAX
                io.write("usesColorPicker", u->gui.usesColorPicker);
                break;
            }
            case vir::Shader::Uniform::Type::Float4 :
            {
                io.write("value", u->getValue<glm::vec4>());
                WRITE_MIN_MAX
                io.write("usesColorPicker", u->gui.usesColorPicker);
                break;
            }
            case vir::Shader::Uniform::Type::Sampler2D :
            case vir::Shader::Uniform::Type::Sampler3D :
            case vir::Shader::Uniform::Type::SamplerCube :
            case vir::Shader::Uniform::Type::Image2D :
            case vir::Shader::Uniform::Type::Image3D :
            case vir::Shader::Uniform::Type::ImageCube :
            {
                auto r = u->getValuePtr<Resource>();
                if (r != nullptr)
                    io.write("value", r->name().c_str());
                break;
            }
            default:
                break;
        }
        io.writeObjectEnd(); // End of 'u->name'
    }
    io.writeObjectEnd(); // End of uniforms
}

}