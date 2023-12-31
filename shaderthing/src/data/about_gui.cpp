#include "data/about.h"
#include "data/data.h"
#include "resources/resource.h"
#include "misc/misc.h"
#include "version.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

void About::renderGui()
{
    if (!isGuiOpen_)
        return;

    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,600), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags
        (
            ImGuiWindowFlags_NoCollapse
        );
        ImGui::Begin("About", &isGuiOpen_, windowFlags);
        static bool setIcon(false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "About", 
                IconData::sTIconData, 
                IconData::sTIconSize,
                false
            );
        }
    }
    float fontSize(ImGui::GetFontSize());
    float textWidth(40.0f*fontSize);
    ImGui::PushTextWrapPos
    (
        isGuiInMenu_ ?
        (ImGui::GetCursorPos().x+textWidth) : 
        ImGui::GetContentRegionAvail().x
    );

    //--------------------------------------------------------------------------
    static std::string nameAndversion
    (
        "ShaderThing - v"+
        std::to_string(VERSION_MAJOR)+"."+
        std::to_string(VERSION_MINOR)+"."+
        std::to_string(VERSION_PATCH)
    );
    ImGui::Text(nameAndversion.c_str());
    ImGui::Text("");

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
    float time = vir::GlobalPtr<vir::Time>::instance()->outerTime();
    for (int i=0; i < nv; i++)
    {
        char& colorChar = colorChars[i%nc];
        colorDict[i].second =
            ImVec4
            {
                .7f+.3f*glm::cos(6.283f*(i)/nv-4.f*time), 
                .5f+.3f*glm::cos(6.283f*(i+float(nv)/3.f)/nv-4.f*time), 
                .5f+.3f*glm::cos(6.283f*(i+float(2.f*nv)/3.f)/nv-4.f*time), 
                1.f
            };
    }
    Misc::OneLineColorfulText(fancyVirmodoetiae.c_str(), colorDict);
    if 
    (
        ImGui::IsItemHovered() && 
        ImGui::BeginTooltip() && 
        virmodoetiaeImage_ != nullptr
    )
    {
        ImGui::Image
        (
            (void*)(uintptr_t)(virmodoetiaeImage_->id()), 
            {256,256}, 
            {0,1}, 
            {1,0}
        );
        ImGui::EndTooltip();
    }
    ImGui::SetCursorPos(pos1);
    ImGui::Text("");

    //--------------------------------------------------------------------------
    ImGui::SeparatorText("License");
    ImGui::Text(
"Copyright © 2023 Stefan Radman (Стефан Радман, a.k.a., virmodoetiae)\n\n"
"This software is provided \"as-is\", without any express or implied warranty. "
"In no event will the author be held liable for any damages arising from the "
"use of this software.\n\n"
"Permission is granted to anyone to use this software for any purpose, "
"including commercial applications, and to alter it and redistribute it "
"freely, subject to the following restrictions:\n");
    ImGui::Text("");
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
    ImGui::Text("");

    //--------------------------------------------------------------------------
    ImGui::SeparatorText("Acknowledgements");
    ImGui::Text(
"ShaderThing would have either taken much longer to come to its present state, "
"or it would have been impossible to develop, if it were not for the "
"availability of the following third-party libraries & projects:");
    ImGui::Text("");

#define ACKNOWLEDGE(lib, linkChar, linkText)                                \
ImGui::Bullet();ImGui::Text(lib);ImGui::SameLine();ImGui::Text(linkChar);   \
if (ImGui::IsItemHovered() && ImGui::BeginTooltip()){                       \
    ImGui::Text(linkText);                                                  \
    ImGui::EndTooltip();}

    ACKNOWLEDGE(
"OpenGL (core profile with support up to v4.60)  ", "[D]", "Dynamically linked")
    ACKNOWLEDGE(
"GLAD - OpenGL loader, v0.1.34                   ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"GLFW - Window & input management, v3.3.8        ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"GLM - OpenGL mathematics, v0.9.9.9              ", "[H]", "Header-only")
    ACKNOWLEDGE(
"STB - Image read/write operations, v2.28        ", "[H]", "Header-only")
    ACKNOWLEDGE(
"ImGui - Immediate mode GUI, v1.89.8-docking     ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"ImGuiFileDialog - ImGui-based file dialog       ", "[S]", "Statically linked")
    ACKNOWLEDGE(
"ColorTextEditor - ImGui-based text editor (mod.)", "[S]", "Statically linked")
    ImGui::Text("");

    ImGui::PopTextWrapPos();
    if (!isGuiInMenu_)
        ImGui::End();
}

}