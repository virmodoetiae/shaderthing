/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_extensions.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

#include "vir/include/vir.h"

#include "shaderthing/include/uniform.h"

namespace ShaderThing
{

typedef Uniform::Type Type;
typedef Uniform::ManagedType ManagedType;

bool Uniform::renderEditBoundsButtonGui(bool renderDragStepSlider)
{
    glm::vec2& bounds = gui.bounds;
    float* dragStep = &gui.dragStep;
    float* logarithmicZero = &gui.logarithmicZero;
    if (ImGui::Button(ICON_FA_RULER_COMBINED, ImVec2(-1, 0)))
        ImGui::OpenPopup("##uniformBounds");
    if (ImGui::BeginPopup("##uniformBounds"))
    {
        
        glm::vec2 bounds0(bounds);
        if (type_ == vir::Uniform::Type::UInt)
            bounds.x = std::max(bounds.x, 0.0f);
        ImGui::Text("Minimum value    ");
        ImGui::SameLine();
        float inputWidth = 6*ImGui::GetFontSize();
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
            (type_ == Type::Int2 || type_ == Type::Float2) && 
            renderDragStepSlider
        )
        {
            auto format = Helpers::getFormat(*dragStep);
            ImGui::Text("Mouse drag step  ");
            ImGui::SameLine();
            ImGui::PushItemWidth(inputWidth);
            if (type_ == Type::Int2)
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
            isLogarithmic &&
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
}

}