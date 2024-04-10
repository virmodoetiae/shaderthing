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


#include <memory>
#include <string>
#include <vector>

#include "shaderthing/include/about.h"
#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/version.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

void About::renderGui()
{

    float fontSize(ImGui::GetFontSize());
    float textWidth(40.0f*fontSize);
    float vSpace = ImGui::GetTextLineHeightWithSpacing();
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x+textWidth);

    //--------------------------------------------------------------------------
    static std::string nameAndVersion
    (
        "ShaderThing - v"+
        std::to_string(VERSION_MAJOR)+"."+
        std::to_string(VERSION_MINOR)+"."+
        std::to_string(VERSION_PATCH)
    );
    ImGui::Text(nameAndVersion.c_str());
    ImGui::Dummy({-1, vSpace});

    //--------------------------------------------------------------------------
    ImGui::SeparatorText("Information");
    auto pos0 = ImGui::GetCursorPos();
    ImGui::Text(
                          // Stefan Radman, nuclear engineer, PhD
"ShaderThing is developed by                                     . " 
"It is primarily an exploratory project, born organically out of the author's "
"desire to deepen his knowledge of OpenGL and shader programming by designing "
"a tool for tinkering with computer graphics.");
    auto pos1 = ImGui::GetCursorPos();
    pos0.x += ImGui::CalcTextSize("ShaderThing is developed by ").x;
    ImGui::SetCursorPos(pos0);

    // One time initialization of formatted colored string
    static std::string colorChars
    (
        "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM"
    );
    static int nc = colorChars.size();
    static std::string virmodoetiae = "Stefan Radman, nuclear engineer, PhD";
    static int nv = virmodoetiae.size();
    static std::vector<std::pair<char, ImVec4>> colorDict(0);
    static std::string fancyVirmodoetiae("");
    if (fancyVirmodoetiae.size() == 0 || colorDict.size() == 0)
    {
        fancyVirmodoetiae.clear();
        colorDict.clear();
        for (int i=0; i < nv; i++)
        {
            char& colorChar = colorChars[i%nc];
            colorDict.emplace_back
            (
                std::pair<char, ImVec4>{colorChar, ImVec4{1.f, 1.f, 1.f, 1.f}}
            );
            fancyVirmodoetiae += "$";
            fancyVirmodoetiae += colorChar;
            fancyVirmodoetiae += virmodoetiae[i];
        }
    }

    // Time & position dependent color
    float time = vir::Time::instance()->outerTime();
    for (int i=0; i < nv; i++)
    {
        colorDict[i].second =
            ImVec4
            {
                .7f+.3f*glm::cos(6.283f*(i)/nv-4.f*time), 
                .5f+.3f*glm::cos(6.283f*(i+float(nv)/3.f)/nv-4.f*time), 
                .5f+.3f*glm::cos(6.283f*(i+float(2.f*nv)/3.f)/nv-4.f*time), 
                1.f
            };
    }
    Helpers::oneLineColorfulText(fancyVirmodoetiae.c_str(), colorDict);

    static auto virmodoetiaeImage = 
        std::unique_ptr<vir::TextureBuffer2D>
        (
            vir::TextureBuffer2D::create
            (
                ByteData::Image::virmodoetiaeData,
                ByteData::Image::virmodoetiaeSize
            )
        );
    if 
    (
        ImGui::IsItemHovered() && 
        ImGui::BeginTooltip()
    )
    {
        ImGui::Image
        (
            (void*)(uintptr_t)(virmodoetiaeImage->id()), 
            {256,256}, 
            {0,1}, 
            {1,0}
        );
        ImGui::EndTooltip();
    }
    ImGui::SetCursorPos(pos1);
    ImGui::Text(" ");

    //--------------------------------------------------------------------------
    ImGui::SeparatorText("License");
    ImGui::Text(
"Copyright © 2024 Stefan Radman (Стефан Радман, a.k.a., virmodoetiae)\n\n"
"This software is provided \"as-is\", without any express or implied warranty. "
"In no event will the author be held liable for any damages arising from the "
"use of this software.\n\n"
"Permission is granted to anyone to use this software for any purpose, "
"including commercial applications, and to alter it and redistribute it "
"freely, subject to the following restrictions:\n");
    ImGui::Dummy({-1, vSpace});
    ImGui::Bullet();ImGui::Text(
"The origin of this software must not be misrepresented; you must not claim "
"that you wrote the original software. If you use this software in a product, "
"an acknowledgment in the product documentation would be appreciated but is "
"not required.");
    ImGui::Bullet();ImGui::Text(
"Altered source versions must be plainly marked as such, and must not be "
"misrepresented as being the original software.");
    ImGui::Bullet();ImGui::Text(
"This notice may not be removed or altered from any source distribution.");
    ImGui::Dummy({-1, vSpace});

    //--------------------------------------------------------------------------
    ImGui::SeparatorText("Acknowledgements");
    ImGui::Text(
"ShaderThing would have either taken much longer to come to its present state, "
"or it would have been impossible to develop, if it were not for the "
"availability of the following third-party libraries & projects:");
    ImGui::Dummy({-1, vSpace});

#define ACKNOWLEDGE(lib, linkChar, linkText)                                \
ImGui::Bullet();ImGui::Text(lib);ImGui::SameLine();ImGui::Text(linkChar);   \
if (ImGui::IsItemHovered() && ImGui::BeginTooltip()){                       \
    ImGui::Text(linkText);                                                  \
    ImGui::EndTooltip();}

    ACKNOWLEDGE(
"OpenGL (v3.3.0 through v4.6.0 supported)         ", "[D]", "Dynamically linked")
    ACKNOWLEDGE(
"GLAD - OpenGL loader, v0.1.34                    ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"GLFW - Window & input management, v3.3.8         ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"GLM - OpenGL mathematics, v0.9.9.9               ", "[H]", "Header-only")
    ACKNOWLEDGE(
"STB - Image read/write operations, v2.28         ", "[H]", "Header-only")
    ACKNOWLEDGE(
"ImGui - Immediate mode GUI, v1.89.8-docking      ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"ColorTextEditor - ImGui-based text editor (mod.) ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"PortableFileDialogs - Native OS dialogs, v0.1.0  ", "[H]", "Header-only")
    ACKNOWLEDGE(
"RapidJSON - JSON file read/write ops., v1.1.0    ", "[H]", "Header-only")
    ImGui::Dummy({-1, vSpace});

    ImGui::PopTextWrapPos();

}

}