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

#include "shaderthing/include/app.h"
#include "shaderthing/include/coderepository.h"
#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/examples.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/shareduniforms.h"
#include "shaderthing/include/statusbar.h"
#include "shaderthing/include/texteditor.h"
#include "shaderthing/include/uniform.h"
#include "shaderthing/include/version.h"

namespace ShaderThing
{

typedef Uniform::Type Type;
typedef Uniform::ManagedType ManagedType;

//----------------------------------------------------------------------------//

void App::renderControlPanelGui()
{
    vir::ImGuiRenderer::newFrame();
    
    ImGui::SetNextWindowSize(ImVec2(768, 768), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin(controlPanelTitle_.c_str(), NULL, flags);

    // Refresh icon if needed
    static bool isIconSet(false);
    static bool isWindowDocked(ImGui::IsWindowDocked());
    if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
    {
        isIconSet = vir::ImGuiRenderer::setWindowIcon
        (
            controlPanelTitle_.c_str(), 
            ByteData::Icon::sTIconData, 
            ByteData::Icon::sTIconSize,
            false
        );
        isWindowDocked = ImGui::IsWindowDocked();
    }
    
    renderMenuBarGui();
    renderLayersTabBarGui();

    ImGui::End();
    
    vir::ImGuiRenderer::render();
}

//----------------------------------------------------------------------------//

void App::renderMenuBarGui()
{
    bool windowIconified = vir::Window::instance()->iconified();
    bool newProjectConfirmation = false;
    bool shadersRequireRecompilation = false;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N", nullptr, !windowIconified))
                newProjectConfirmation = true;
            if (ImGui::MenuItem("Load", "Ctrl+O", nullptr, !windowIconified))
                onLoadCallback();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                onSaveCallback();
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
                onSaveAsCallback();
            ImGui::Separator();
            if (ImGui::BeginMenu("Export"))
            {
                renderExportMenuGui();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Properties"))
        {   
            renderWindowPropertiesMenuGui();
            for (auto& layer : layers)
                layer->renderMenuGui();
            ImGui::Separator();
            renderShaderLanguangeExtensionsMenuGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            renderResourcesMenuGui();
            shadersRequireRecompilation = 
                sharedStorage->renderMenuItemGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Find"))
        {
            TextEditor::renderFindReplaceToolMenuGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences"))
        {
            renderFontPreferencesMenuGui();
            renderAutoSavePreferencesMenuGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            CodeRepository::renderMenuItemGui();
            renderExamplesMenuGui();
            renderAboutShaderThingMenuGui();
            renderSystemInfoMenuGui();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (Resource::gui.isDetachedFromControlPanel)
        renderResourcesTableGui();
    if (sharedStorage->isGuiDetachedFromMenu())
        shadersRequireRecompilation = 
            sharedStorage->renderGui();
    if (CodeRepository::isDetachedFromMenu)
        CodeRepository::renderGui();
    
    if (Helpers::isCtrlKeyPressed(ImGuiKey_N) && !windowIconified)
        newProjectConfirmation = true;
    else if (Helpers::isCtrlKeyPressed(ImGuiKey_O) && !windowIconified)
        onLoadCallback();
    else if (Helpers::isCtrlShiftKeyPressed(ImGuiKey_S))
        onSaveAsCallback();
    else if (Helpers::isCtrlKeyPressed(ImGuiKey_S))
        onSaveCallback();

    if (shadersRequireRecompilation)
        deferredActionBuffer.add
        (
            [this](){for (auto& layer : layers){layer->compileShader();}}
        );

    if (newProjectConfirmation)
        ImGui::OpenPopup("New project confirmation");
    if 
    (
        ImGui::BeginPopupModal
        (
            "New project confirmation", 
            nullptr, 
            ImGuiWindowFlags_NoResize
        )
    )
    {
        ImGui::Text("Are you sure you want to start a new project?");
        ImGui::Text("Any unsaved edits to the current project will be lost!");
        if (ImGui::Button("Confirm"))
        {
            deferredActionBuffer.add([this](){initialize();});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void App::renderWindowPropertiesMenuGui()
{
    if (ImGui::BeginMenu("Window", !vir::Window::instance()->iconified()))
    {
        auto& su = *(sharedUniforms);
        ImGui::Text("Resolution         ");
        ImGui::SameLine();
        ImGui::PushItemWidth(8.0*ImGui::GetFontSize());
        glm::ivec2 resolution(su.iResolution);
        if 
        (
            ImGui::InputInt2
            (
                "##windowResolution", 
                glm::value_ptr(resolution)
            )
        )
            setWindowResolution(resolution,false);
        ImGui::PopItemWidth();
        auto window = vir::Window::instance();
        ImGui::Text("VSync              ");
        ImGui::SameLine();
        if 
        (
            ImGui::Checkbox
            (
                "##windowVSync", 
                &renderState.isVSyncEnabled
            )
        )
            window->setVSync(renderState.isVSyncEnabled);

        ImGui::Text("GUI fps multiplier ");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text(
R"(Frame rate multiplier of the graphical user interface (GUI). By default, the 
GUI frame rate is tied to the shader rendering frame rate in the main window. 
When rendering computationally intensive shaders, the GUI frame rate is affected
as well, resulting in a worsened user experience. Set this multiplier to values
larger than one to recover the GUI frame rate, at the expense, however, of a
further reduction of the shader rendering frame rate)");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(8.f*ImGui::GetFontSize());
        int nRenderingTiles = renderState.nTiles;
        if (renderState.isPaused)
            ImGui::BeginDisabled();
        if (ImGui::InputInt("##nRenderingTiles", &nRenderingTiles))
        {
            setRenderingTiles(nRenderingTiles);
        }
        if (renderState.isPaused)
            ImGui::EndDisabled();
        
        ImGui::Text("Pause render below ");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text(
R"(Frame rate of the graphical user interface (GUI) below which shader rendering
is paused, to prevent e.g., making the app unresponsive should the shader(s) be 
accidentally made too computationally intensive. This feature is disabled during 
project exports)");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(5.0*ImGui::GetFontSize());
        if 
        (
            ImGui::InputFloat
            (
                "##maxLowFps", 
                &renderState.lowerFpsLimit, 
                0.f, 
                0.f, 
                "%.1f"
            )
        )
            renderState.lowerFpsLimit = 
                std::max(renderState.lowerFpsLimit, 0.f);
        ImGui::SameLine();
        ImGui::PopItemWidth();
        ImGui::Text("fps");

        if 
        (
            ImGui::Button
            (
                !renderState.isPaused ? 
                "Pause rendering" : "Resume rendering", 
                ImVec2(-1, 0)
            )
        )
            toggleRenderingPaused(false);

        if (ImGui::Button("Capture mouse cursor", ImVec2(-1, 0)))
            setMouseCaptured(true);

        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderShaderLanguangeExtensionsMenuGui()
{
    if (ImGui::BeginMenu("OpenGL extensions"))
    {
        float fontSize(ImGui::GetFontSize());
        float textWidth(40.0f*fontSize);
        float vSpace = .25*ImGui::GetTextLineHeightWithSpacing();
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x+textWidth);
        ImGui::Text(
"List of all OpenGL extensions supported by your system. To enable or disable "
"an extension, click on its checkbox. Please note that: ");
        ImGui::Dummy(ImVec2(-1, vSpace));
        ImGui::Bullet();ImGui::Text(
"some of the listed extensions may only affect the back-end API and not expose "
"new features at the GLSL level;");
        ImGui::Dummy(ImVec2(-1, vSpace));
        ImGui::Bullet();ImGui::Text(
"some of the listed extensions may expose new features at the GLSL level, but "
"not at the fragment shader stage level, which is the only stage over which "
"ShaderThing gives you control;");
        ImGui::Dummy(ImVec2(-1, vSpace));
        ImGui::Bullet();ImGui::Text(
"some of the listed extensions may already be enabled by default by your current "
"context (%s), yet their status is never correctly reported and there is no "
"possibility of disabling them: enabling/disabling such extensions from here is "
"inconsequential.", 
        vir::Window::instance()->context()->name().c_str());
        ImGui::Dummy(ImVec2(-1, vSpace));
        ImGui::Text(
"The previously listed issues are due to the impossibility to query the "
"graphics driver for detailed extension status information. The user is "
"directed to the KhronosOpenGL Registry for further information regarding "
"specific extensions");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(-1, vSpace));

        static std::string filter = "";
        static bool caseSensitive = false;
        const auto& supportedExtensions = 
            vir::Window::instance()->context()->supportedExtensions();
        static std::vector<std::string> filteredExtensions = 
            supportedExtensions;

        ImGui::Text("Filter extensions by name");
        ImGui::SameLine();
        bool caseSensitiveChanged = false;
        if 
        (
            ImGui::SmallButton
            (
                caseSensitive ? ICON_FA_CHECK " Aa" : ICON_FA_BAN " Aa"
            )
        )
        {
            caseSensitive = !caseSensitive;
            caseSensitiveChanged = true;
        }
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            if (caseSensitive)
                ImGui::Text("Filter is case-sensitive");
            else
                ImGui::Text("Filter is not case-sensitive");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if 
        (
            ImGui::InputText("##extensionFilter", &filter) || 
            caseSensitiveChanged
        )
        {
            filteredExtensions.clear();
            if (filter.size() > 0)
            {
                std::string lFilter = filter;
                if (!caseSensitive)
                    std::transform
                    (
                        lFilter.begin(), 
                        lFilter.end(), 
                        lFilter.begin(), 
                        ::tolower
                    );
                for (const auto& extension : supportedExtensions)
                {
                    std::string lExtension = extension;
                    if (!caseSensitive)
                        std::transform
                        (
                            lExtension.begin(), 
                            lExtension.end(), 
                            lExtension.begin(), 
                            ::tolower
                        );
                    if (lExtension.find(lFilter) != std::string::npos)
                        filteredExtensions.push_back(extension);
                }
            }
            else
                filteredExtensions = supportedExtensions;
        }
        ImGui::PopItemWidth();
        ImGui::PopTextWrapPos();
        ImGui::BeginChild
        (
            "##graphicsContextExtensionManager", 
            ImVec2(0, 12*4*vSpace),
            true
        );
        
        int id = 0;
        for (auto extension : filteredExtensions)
        {
            bool status = 
                vir::Shader::
                isExtensionInCurrentContextShadingLanguageDirectives
                (
                    extension
                );
            ImGui::PushID(id++);
            if (ImGui::Checkbox("##extensionStatus", &status))
            {
                vir::Shader::
                setExtensionStatusInCurrentContextShadingLanguageDirectives
                (
                    extension, status
                );
                for (auto& layer : layers)
                    layer->compileShader();
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text(extension.c_str());
        }
        ImGui::EndChild();
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderFontPreferencesMenuGui()
{
    if (ImGui::BeginMenu("Font"))
    {
        ImGui::Text("%s", "Scale");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat
        (
            "##fontScale",
            font_.scale,
            0.005f,
            0.5f,
            1.0f,
            "%.1f"
        );
        ImGui::PopItemWidth();
        ImGui::Separator();
        if (ImGui::BeginMenu("Load character sets"))
        {
            bool alwaysBuiltSet(true);
            
            ImGui::Text("%s", "Latin            ");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##loadLatin", &alwaysBuiltSet);
            ImGui::EndDisabled();

            ImGui::Text("%s", "Cyrillic         ");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##loadCyrillic", &alwaysBuiltSet);
            ImGui::EndDisabled();

            ImGui::Text("%s", "Greek            ");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##loadGreek", &alwaysBuiltSet);
            ImGui::EndDisabled();

            ImGui::Text("%s", "Japanese         ");
            ImGui::SameLine();
            bool japaneseDisabled = font_.isJapaneseLoaded;
            if (japaneseDisabled)
                ImGui::BeginDisabled();
            if (ImGui::Checkbox("##loadJapanese", &font_.isJapaneseLoaded))
                deferredActionBuffer.add
                (
                    [this]()
                    {
                        auto& io = ImGui::GetIO();
                        io.Fonts->AddFontFromMemoryCompressedTTF
                        ( 
                            (void*)ByteData::Font::DroidSansFallbackData, 
                            ByteData::Font::DroidSansFallbackSize,
                            36.5,
                            &font_.imFontConfig, 
                            io.Fonts->GetGlyphRangesJapanese()
                        );
                        io.Fonts->Build();
                        vir::ImGuiRenderer::destroyDeviceObjects();
                        font_.isJapaneseLoaded = true;
                    }
                );
            if (japaneseDisabled)
                ImGui::EndDisabled();

            ImGui::Text("%s", "Chinese (simpl.) ");
            ImGui::SameLine();
            bool simplifiedChineseDisabled = font_.isSimplifiedChineseLoaded;
            if (simplifiedChineseDisabled)
                ImGui::BeginDisabled();
            if 
            (
                ImGui::Checkbox
                (
                    "##loadSimplifiedChinese", 
                    &font_.isSimplifiedChineseLoaded
                )
            )
                deferredActionBuffer.add
                (
                    [this]()
                    {
                        auto& io = ImGui::GetIO();
                        io.Fonts->AddFontFromMemoryCompressedTTF
                        ( 
                            (void*)ByteData::Font::DroidSansFallbackData, 
                            ByteData::Font::DroidSansFallbackSize,
                            36.5,
                            &font_.imFontConfig, 
                            io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
                        );
                        io.Fonts->Build();
                        vir::ImGuiRenderer::destroyDeviceObjects();
                        font_.isSimplifiedChineseLoaded = true;
                    }
                );
            if (simplifiedChineseDisabled)
                ImGui::EndDisabled();

            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderAutoSavePreferencesMenuGui()
{
    if (ImGui::BeginMenu("Auto-save"))
    {
        bool canBeEnabled = project_.filepath.size() > 0;
        if (!canBeEnabled)
            ImGui::BeginDisabled();
        ImGui::Text("Enabled  ");
        ImGui::SameLine();
        if (!canBeEnabled)
        {
            bool flag(false);
            ImGui::Checkbox("##checkAutoSave", &flag);
            if 
            (
                !canBeEnabled && 
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::EndDisabled();
                ImGui::Text
                (
R"(The auto-save feature can be enabled only once the
project has been manually saved for the first time)"
                );
                ImGui::BeginDisabled();
                ImGui::EndTooltip();
            }
        }
        else
            ImGui::Checkbox("##checkAutoSave", &project_.isAutoSaveEnabled);
        if (canBeEnabled && !project_.isAutoSaveEnabled)
            ImGui::BeginDisabled();
        ImGui::Text("Interval ");
        ImGui::SameLine();
        static int index(1);
        static float intervals[5] {.5f, 1.f, 2.f, 5.f, 10.f};
        if (int(intervals[index]*60) != int(project_.autoSaveInterval))
        {
            for (index=0; index<5; index++)
            {
                if (project_.autoSaveInterval <= intervals[index]*60.f)
                    break;
            }
        }
        if (ImGui::SmallButton(ICON_FA_MINUS))
            index = std::max(index-1, 0);
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_PLUS))
            index = std::min(index+1, 4);
        ImGui::SameLine();
        if (index == 0)
            ImGui::Text("30 sec");
        else
            ImGui::Text("%d min", int(intervals[index]));
        project_.autoSaveInterval = intervals[index]*60.f;
        if (!canBeEnabled || !project_.isAutoSaveEnabled)
            ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderExamplesMenuGui()
{
    if (ImGui::BeginMenu("Examples"))
    {
        bool loadExampleConfirmation = false;
        static const std::string* preSelection = nullptr;
        ImGui::Text
        (
R"(This is a collection of built-in project examples which 
can be freely loaded, edited and used for learning 
purposes or as starting points for other projects)"
        );
        ImGui::Separator();

        int id = 0;

        //--------------------------------- Bloom
        /* TODO Re-enable once post-processing effects are available
        ImGui::PushID(id++);
        static std::string errorMessage;
        static auto canRunOnDeviceInUse = []() -> bool
        {
            auto nativeBloom = vir::Bloomer::create();
            bool result = nativeBloom->canRunOnDeviceInUse();
            errorMessage = std::string(nativeBloom->errorMessage());
            return result;
        };
        static bool bloom(canRunOnDeviceInUse());
        if (!bloom)
            ImGui::BeginDisabled();
        if (ImGui::Button("Load"))
        {
            loadExampleConfirmation = true;
            tmpSelection = &Examples::bloom0;
        }
        if (!bloom)
        {
            ImGui::EndDisabled();
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text(errorMessage.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::Text("Bloom (post-processing effect)");
        */
        
        //--------------------------------- Ray marching
        ImGui::PushID(id++);
        if (ImGui::Button("Load"))
        {
            loadExampleConfirmation = true;
            preSelection = &Examples::rayMarching;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::Text("Ray marcher (basic)");

        //--------------------------------- Path marching
        ImGui::PushID(id++);
        if (ImGui::Button("Load"))
        {
            loadExampleConfirmation = true;
            preSelection = &Examples::pathMarching;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::Text("Monte Carlo path marcher (advanced)");

        //----------------------------------------------------------------------
        if (loadExampleConfirmation)
            ImGui::OpenPopup("Load example project confirmation");
        if 
        (
            ImGui::BeginPopupModal
            (
                "Load example project confirmation", 
                nullptr, 
                ImGuiWindowFlags_NoResize
            )
        )
        {
            ImGui::Text("Are you sure you want to load the selected project?");
            ImGui::Text("Any unsaved edits to the current project will be lost!");
            if (ImGui::Button("Confirm"))
            {
                auto selection = preSelection;
                deferredActionBuffer.add
                (
                    [selection, this]()
                    {
                        project_.forceSaveAs = true;
                        project_.filepath = Project{}.filepath;
                        project_.filename = Project{}.filename;
                        loadFrom(*selection, true);
                    }
                );
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                preSelection = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderAboutShaderThingMenuGui()
{
    if (ImGui::BeginMenu("About ShaderThing"))
    {
        float fontSize(ImGui::GetFontSize());
        float textWidth(40.0f*fontSize);
        float vSpace = .25*ImGui::GetTextLineHeightWithSpacing();
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x+textWidth);

        //----------------------------------------------------------------------
        static std::string nameAndVersion
        (
            "ShaderThing - v"+
            std::to_string(VERSION_MAJOR)+"."+
            std::to_string(VERSION_MINOR)+"."+
            std::to_string(VERSION_PATCH)
        );
        ImGui::Text(nameAndVersion.c_str());
        ImGui::Dummy({-1, vSpace});

        //----------------------------------------------------------------------
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
                    std::pair<char, ImVec4>
                    {
                        colorChar, 
                        ImVec4{1.f, 1.f, 1.f, 1.f}
                    }
                );
                fancyVirmodoetiae += "$";
                fancyVirmodoetiae += colorChar;
                fancyVirmodoetiae += virmodoetiae[i];
            }
        }

        // Time & position dependent color
        float time = vir::Window::instance()->time()->outerTime();
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
            vir::TextureBuffer2D::create
            (
                ByteData::Image::virmodoetiaeData,
                ByteData::Image::virmodoetiaeSize
            );
        if 
        (
            ImGui::IsItemHovered() && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Image
            (
                (ImTextureID)(virmodoetiaeImage->id()), 
                {256,256}, 
                {0,1}, 
                {1,0}
            );
            ImGui::EndTooltip();
        }
        ImGui::SetCursorPos(pos1);
        ImGui::Dummy({-1, 4*vSpace});

        //----------------------------------------------------------------------
        ImGui::SeparatorText("License");
        ImGui::Text(
"Copyright © 2025 Stefan Radman (Стефан Радман, a.k.a., virmodoetiae)");
        ImGui::Dummy({-1, vSpace});
        ImGui::Text(
"This software is provided \"as-is\", without any express or implied warranty. "
"In no event will the author be held liable for any damages arising from the "
"use of this software.");
        ImGui::Dummy({-1, vSpace});
        ImGui::Text(
"Permission is granted to anyone to use this software for any purpose "
"(including commercial applications) that is not hereby prohibited, and to "
"alter it and redistribute it freely, subject to the following restrictions:");
        ImGui::Dummy({-1, vSpace});
        ImGui::Bullet(); ImGui::Text(
"This software, or any derivative works, must not be used to violate or "
"promote ideas that violate the sovereignity or the territorial integrity of "
"the Repulic of Serbia. Furthermore, this software, or any derivative works, "
"must not be used to harm or defame or promote ideas that harm or defame the "
"Serbian nation. ");
        ImGui::Dummy({-1, vSpace});
        ImGui::Bullet(); ImGui::Text(
"The origin of this software must not be misrepresented; you must not claim "
"that you wrote the original software. If you use this software in a product, "
"an acknowledgment in the product documentation would be appreciated but is "
"not required.");
        ImGui::Dummy({-1, vSpace});
        ImGui::Bullet(); ImGui::Text(
"Altered source versions must be plainly marked as such, and must not be "
"misrepresented as being the original software.");
        ImGui::Dummy({-1, vSpace});
        ImGui::Bullet(); ImGui::Text(
"This notice may not be removed or altered from any source distribution.");
        ImGui::Dummy({-1, 4*vSpace});

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
"GLFW - Window & input management, v3.4           ", "[S]", "Statically linked")
        ACKNOWLEDGE(
"GLM - OpenGL mathematics, v0.9.9.9               ", "[H]", "Header-only")
        ACKNOWLEDGE(
"STB - Image read/write operations, v2.28         ", "[H]", "Header-only")
        ACKNOWLEDGE(
"Dear ImGui - Immediate mode GUI, v1.91.5-docking ", "[S]", "Statically linked")
        ACKNOWLEDGE(
"ColorTextEditor - ImGui-based text editor (mod.) ", "[S]", "Statically linked")
        ACKNOWLEDGE(
"PortableFileDialogs - Native OS dialogs, v0.1.0  ", "[H]", "Header-only")
        ACKNOWLEDGE(
"RapidJSON - JSON file read/write ops., v1.1.0    ", "[H]", "Header-only")
        ImGui::Dummy({-1, vSpace});
        ImGui::PopTextWrapPos();

        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderSystemInfoMenuGui()
{
    if (ImGui::BeginMenu("System info"))
    {
        ImGui::Text("%s", "Graphics card in use:");
        ImGui::SameLine();
        ImGui::Text
        (
            "%s", 
            vir::Renderer::instance()->
            deviceName().c_str()
        );
        ImGui::Text("%s", "Graphics context:    ");
        ImGui::SameLine();
        ImGui::Text
        (
            "%s", 
            vir::Window::instance()->
            context()->name().c_str()
        );
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderLayersTabBarGui()
{
    layersHaveUncompiledEdits = false;
    layersHaveCompilationErrors = false;
    for (auto& layer : layers)
    {
        layersHaveUncompiledEdits = 
            layersHaveUncompiledEdits ||
            layer->hasUncompiledEdits;
        layersHaveCompilationErrors = 
            layersHaveCompilationErrors ||
            layer->hasCompilationErrors();
    }
    if 
    (
        layersHaveUncompiledEdits || 
        layersHaveCompilationErrors
    ) // Render compilation button 
    {
        float time = vir::Window::instance()->time()->outerTime();
        ImVec4 compileButtonColor = 
        {
            .5f*glm::sin(6.283f*(time/3+0.f/3))+.3f,
            .5f*glm::sin(6.283f*(time/3+1.f/3))+.3f,
            .5f*glm::sin(6.283f*(time/3+2.f/3))+.3f,
            1.f
        };
        ImGui::PushStyleColor(ImGuiCol_Button, compileButtonColor);
        if 
        (
            ImGui::ArrowButton("##right",ImGuiDir_Right) ||
            Helpers::isCtrlKeyPressed(ImGuiKey_B)
        )
        {
            ImGui::SetTooltip("Compiling project shaders...");
            for (auto& layer : layers)
            {
                layer->compileShader();
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("Compile all shaders");
        ImGui::SameLine();
        static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
        ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
        ImGui::Text("Ctrl+B");
        ImGui::PopStyleColor();
    }

    // Render list of compilation errors with formatting -----------------------
    bool errorColorPushed = false;
    if (layersHaveCompilationErrors)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, {1,0,0,1});
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    Layer::renderSharedCompilationErrorsGui();
    for (auto& layer : layers) 
    {
        layer->renderCompilationErrorsGui();
    }
    if (layersHaveCompilationErrors)
        ImGui::Separator();
    if (errorColorPushed)
        ImGui::PopStyleColor();

    // Actual layers tab bar ---------------------------------------------------
    static bool reorderable = true;
    if 
    (
        ImGui::BeginTabBar
        (
            "##layersTabBar", 
            reorderable ? 
            ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable :
            ImGuiTabBarFlags_AutoSelectNewTabs
        )
    )
    {
        reorderable = true;
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            createNewLayer();
        auto tabBar = ImGui::GetCurrentTabBar();
        std::pair<unsigned int, unsigned int> swap {0,0};
        for (int i = 0; i < (int)layers.size(); i++)
        {
            bool open = true;
            auto& layer = layers[i];
            std::string tabLabel = layer->name()+"###"+layer->imGuiTabId;
            if
            (
                ImGui::BeginTabItem
                (
                    tabLabel.c_str(), 
                    &open, 
                    layers.size() == 1 ? ImGuiTabItemFlags_NoCloseButton : 0
                )
            )
            {
                layer->renderTabGui();
                ImGui::EndTabItem();
            }
            if (!open) // I.e., if 'x' is pressed to delete the tab
            {
                ImGui::OpenPopup("Layer deletion confirmation");
                layer->isDeletionConfirmationPending = true;
                // For some reason, when the 'x' is pressed, the tab in question
                // is moved 
                reorderable = false;
            }
            if 
            (
                layer->isDeletionConfirmationPending && 
                ImGui::BeginPopupModal
                (
                    "Layer deletion confirmation",
                    0,
                    ImGuiWindowFlags_NoResize
                )
            )
            {
                ImGui::Text
                (
                    "Are you sure you want to delete '%s'?", 
                    layer->name().c_str()
                );
                ImGui::Text("This action cannot be undone!");
                if (ImGui::Button("Delete"))
                {
                    deferredActionBuffer.add
                    (
                        [&layer, this]()
                        {
                            layers.erase
                            (
                                std::remove
                                (
                                    layers.begin(), 
                                    layers.end(), 
                                    layer
                                ), 
                                layers.end()
                            );
                        }
                    );
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    layer->isDeletionConfirmationPending = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
                continue;
            }
            auto tab = ImGui::TabBarFindTabByID
            (
                tabBar, 
                ImGui::GetID(tabLabel.c_str())
            );
            int order = std::min
            (
                ImGui::TabBarGetTabOrder(tabBar, tab),
                (int)layers.size()-1
            );
            if (order != i)
                swap = {order, i};
        }
        if (swap.first != swap.second)
        {
            const auto d1 = layers[swap.first]->depth();
            const auto d2 = layers[swap.second]->depth();
            std::swap
            (
                layers[swap.first], 
                layers[swap.second]
            );
            layers[swap.first]->setDepth(d1);
            layers[swap.second]->setDepth(d2);
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

#define START_ROW(row, column)                                          \
    ImGui::PushID(row);                                                 \
    ImGui::TableNextRow(0, 1.6*ImGui::GetFontSize());                   \
    column = 0;
#define END_ROW(row)                                                    \
    ImGui::PopID();                                                     \
    ++row;
#define START_COLUMN(column)                                            \
    ImGui::TableSetColumnIndex(column);                                 \
    ImGui::PushItemWidth(-1);
#define END_COLUMN(column)                                              \
    ++column;                                                           \
    ImGui::PopItemWidth();
#define NEXT_COLUMN(column)                                             \
    ImGui::TableSetColumnIndex(column++);

//----------------------------------------------------------------------------//
// Uniforms ------------------------------------------------------------------//
//----------------------------------------------------------------------------//

void App::renderUniformsTabGui(Layer* layer)
{
    //--------------------------------------------------------------------------
    auto& su = *(sharedUniforms);
    float fontSize = ImGui::GetFontSize();
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

    std::string uniformFrameName = "##uniformsFrame"+std::to_string(layer->id);
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
        
        int row = 0;
        int column = 0;

        // First, render the built-in shared uniforms
        if (showSharedAndDefaultUniforms)
            row = renderBuiltInSharedUniformsGui();

        // Then, render the user-created shared uniforms
        int nSharedUniforms = 
            su.fragment.uniforms.size()-
            su.userUniformsStartIndex;
        for (int i=0; i < nSharedUniforms; i++)
        {
            auto& uniform = su.fragment.uniforms
            [
                i + su.userUniformsStartIndex
            ];
            renderUniformTableRowGui
            (
                uniform,
                layer,
                row++,
                i == nSharedUniforms-1
            );
        }

        // Finally, render the user-created layer-specific uniforms
        for(unsigned int i=0; i<layer->fragment.uniforms.size(); i++)
        {
            auto& uniform = layer->fragment.uniforms[i];
            renderUniformTableRowGui
            (
                uniform,
                layer,
                row++,
                false,
                showSharedAndDefaultUniforms
            );
        }

        // Render the "Create new uniform" button
        START_ROW(row, column)
        START_COLUMN(column)
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1, 0)))
        {
            auto& u = Uniform::create(&layer->fragment);
            layer->cache.uncompiledUniforms.emplace_back(u.getWeak());
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
        END_COLUMN(column)
        END_ROW(row)

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
}

//----------------------------------------------------------------------------//

// Render the default/built-in shared uniforms as a table and return the row
// count
int App::renderBuiltInSharedUniformsGui()
{
    auto& su = *(sharedUniforms);
    int row = 0;
    int column;
    float fontSize = ImGui::GetFontSize();
    float halfButtonSize(1.7*fontSize);

    // iFrame --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_UNDO, ImVec2(halfButtonSize, 0)))
    {
        // Restart rendering
        deferredActionBuffer.add
        (
            [this]()
            {
                renderState.frameIndex = 0;
                if (sharedUniforms->isTimeResetOnFrameCounterReset)
                    sharedUniforms->iTime = 0;
                for (auto& layer : layers)
                {
                    layer->clearFramebuffers();
                }
            }
        );
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
            renderState.isPaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        )
    )
    {
        toggleRenderingPaused(false);
        // When stopping rendering while tiled rendering is enabled,
        // make sure to render all the tiles to reach the end of the
        // shader frame
        if
        (
            renderState.isPaused && 
            renderState.isTiledRenderingEnabled
        )
            renderState.toggles.stepToNextFrame = true;
    }
    if (renderState.isPaused)
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
            renderState.toggles.stepToNextFrame = true;
        else
        {
            // If tiled rendering is enabled, stepping by one shader frame
            // means stepping by nTiles app frames (since each app frame 
            // only renders a single shader tile), so the frame step is over
            // only once all tiles have been rendered (i.e., when tileIndex
            // is reset to 0)
            if (renderState.isTiledRenderingEnabled)
            {
                if (renderState.tileIndex == 0)
                    renderState.toggles.stepToNextFrame = false;
            }
            else
                renderState.toggles.stepToNextFrame = false;
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
    NEXT_COLUMN(column)
    ImGui::Text("iFrame");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::UInt].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text("%d", renderState.frameIndex);
    END_ROW(row)

    // iTime --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isTimeLooped ?
            ICON_FA_INFINITY : 
            ICON_FA_CIRCLE_NOTCH,
            ImVec2(halfButtonSize, 0)
        )
    )
        su.isTimeLooped = 
            !su.isTimeLooped;
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        ImGui::Text
        (
            su.isTimeLooped ? 
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
            su.isTimePaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        ) && !renderState.isPaused
    )
        su.isTimePaused = 
            !su.isTimePaused;
    if (su.isTimePaused)
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
            renderState.toggles.stepToNextFrame = true;
        else 
            renderState.toggles.stepToNextFrame = false;
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
    NEXT_COLUMN(column)
    ImGui::Text("iTime");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    glm::vec2* bounds = &su.iTimeUniform->gui.bounds;
    bool boundsChanged = su.iTimeUniform->renderEditBoundsButtonGui();
    NEXT_COLUMN(column)
    auto iTimePtr = &su.iTime;
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
        su.iUserAction = true;
        su.toggles.updateDataRangeII = true;
    }
    ImGui::PopItemWidth();
    END_ROW(row)

    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isTimeResetOnFrameCounterReset ? 
            ICON_FA_BAN " " ICON_FA_UNDO: 
            ICON_FA_CHECK " " ICON_FA_UNDO, 
            ImVec2(-1, 0)
        )
    )
        su.isTimeResetOnFrameCounterReset =
            !su.isTimeResetOnFrameCounterReset;
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        if (su.isTimeResetOnFrameCounterReset)
            ImGui::Text("Disable time reset on rendering restart");
        else
            ImGui::Text("Enable time reset on rendering restart");
        ImGui::EndTooltip();
    }
    END_ROW(row)

    // iTimeDelta --------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isTimeDeltaSmooth ?
            ICON_FA_WAVE_SQUARE : 
            ICON_FA_SIGNATURE, 
            ImVec2(-1, 0)
        )
    )
        su.isTimeDeltaSmooth =
            !su.isTimeDeltaSmooth;
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        if (su.isTimeDeltaSmooth)
            ImGui::Text("Disable time step smoothing");
        else
            ImGui::Text("Enable time step smoothing");
        ImGui::EndTooltip();
    }
    // No actions
    NEXT_COLUMN(column)
    ImGui::Text("iTimeDelta");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    if 
    (
        renderState.isPaused || 
        su.isTimePaused
    )
    {
        ImGui::InputFloat
        (
            "##iTimeDeltaSliderFloat", 
            &su.iTimeDelta,
            0,
            0,
            "%.6f"
        );
        su.iTimeDelta = 
            std::max(su.iTimeDelta, 0.f);
        ImGui::SameLine();
        ImGui::Text("s");
    }
    else
        ImGui::Text("%.6f s", su.iTimeDelta);
    END_ROW(row)
    ImGui::Dummy({0, 0.1f*fontSize});

    // iRandom -------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isRandomNumberGeneratorPaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        )
    )
        su.isRandomNumberGeneratorPaused = 
            !su.isRandomNumberGeneratorPaused;
    NEXT_COLUMN(column)
    ImGui::Text("iRandom");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text("%.6f", su.iRandom);
    END_ROW(row)

    // iWindowAspectRatio --------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    // No actions
    NEXT_COLUMN(column)
    ImGui::Text("iWindowAspectRatio");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text
    (
        "%.3f", su.iAspectRatio
    );
    END_ROW(row)
    ImGui::Dummy({0, 0.1f*fontSize});

    // iWindowResolution ---------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    // No actions
    NEXT_COLUMN(column)
    ImGui::Text("iWindowResolution");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Int2].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text
    (
        "%d x %d", 
        (int)su.iResolution.x, 
        (int)su.iResolution.y
    );
    END_ROW(row)
    
    // iKeyboard -----------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isKeyboardInputEnabled ? 
            ICON_FA_PAUSE : 
            ICON_FA_PLAY, 
            ImVec2(-1, 0)
        )
    )
        toggleKeyboardInputs();
    NEXT_COLUMN(column)
    ImGui::Text("iKeyboard");
    NEXT_COLUMN(column)
    ImGui::Text("vec3[]");
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    std::string pressed = "Pressed:";
    std::string held    = "Held:   ";
    std::string toggled = "Toggled:";
    for (int key=0; key<255; key++)
    {
        auto& keyData(su.iKeyboard[key]);
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
    END_ROW(row)
    
    // iMouse --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isMouseInputEnabled ? 
            ICON_FA_PAUSE : 
            ICON_FA_PLAY, 
            ImVec2(-1, 0)
        )
    )
        toggleMouseInputs();
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iMouseSettings");
    if (ImGui::BeginPopup("##iMouseSettings"))
    {
        bool enabled = su.isMouseInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleMouseInputs();
        ImGui::Text("Clamp value to window resolution ");
        ImGui::SameLine();
        bool status = su.isMouseInputClampedToWindow;
        ImGui::Checkbox("##iMouseSettings_ClampValue", &status);
        if (status != su.isMouseInputClampedToWindow)
            setMouseInputsClamped(status);
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
            &(su.mouseInputRequiresLMBHold)
        );
        ImGui::EndPopup();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iMouse");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float4].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text
    (
        "%d, %d, %d, %d", 
        (int)su.iMouse.x, 
        (int)su.iMouse.y, 
        (int)su.iMouse.z, 
        (int)su.iMouse.w
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
    END_ROW(row)

    // iLook ---------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iLookSettings");
    if (ImGui::BeginPopup("##iLookSettings"))
    {
        bool enabled = su.isCameraMouseInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleCameraMouseInputs();
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
            &(su.cameraMouseInputRequiresLMBHold)
        );
        ImGui::Text("Mouse sensitivity ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat
        (
            "##iLookSensitivity", 
            &su.shaderCamera->mouseSensitivityRef(),
            1e-3,
            1
        );
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iLook");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
    NEXT_COLUMN(column)
    // All cmpts always bounds in [-1, 1]
    NEXT_COLUMN(column)
    {
        glm::vec3 value = su.iLook;
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
            su.iLook = value;
            su.shaderCamera->setDirection(value);
            su.iUserAction = true;
            su.toggles.updateDataRangeII = true;
        }
        ImGui::PopItemWidth();
    }
    END_ROW(row)

    // iWASD ---------------------------------------------------------------
    float posY = 0;
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iWASDSettings");
    if (ImGui::BeginPopup("##iWASDSettings"))
    {
        bool enabled = su.isCameraKeyboardInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleCameraKeyboardInputs();
        ImGui::Text("Keyboard sensitivity ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat
        (
            "##iWASDSensitivity", 
            &su.shaderCamera->keySensitivityRef(),
            1e-1,
            50
        );
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }

    bool showSeparator
    (
        su.fragment.uniforms.size() == 
        su.userUniformsStartIndex
    );
    if (showSeparator)
    {
        posY = ImGui::GetCursorPosY();
        ImGui::Separator();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iWASD");
    if (showSeparator)
    {
        ImGui::SetCursorPosY(posY);
        ImGui::Separator();
    }
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
    if (showSeparator)
    {
        ImGui::SetCursorPosY(posY);
        ImGui::Separator();
    }
    NEXT_COLUMN(column)
    bounds = &su.iWASDUniform->gui.bounds;
    boundsChanged = su.iWASDUniform->renderEditBoundsButtonGui();
    if (showSeparator)
        ImGui::Separator();
    NEXT_COLUMN(column)
    {
        glm::vec3 value = su.iWASD;
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
            su.iWASD = value;
            su.shaderCamera->setPosition(value);
            su.iUserAction = true;
            su.toggles.updateDataRangeII = true;
        }
        ImGui::PopItemWidth();
    }
    if (showSeparator)
        ImGui::Separator();
    END_ROW(row)
    return row;
}

//----------------------------------------------------------------------------//

bool App::renderUniformTableRowGui
(
    UPtr<Uniform>& uniform,
    Layer* layer,
    int row,
    const bool showSeparator,
    const bool showSharedAndDefaultUniforms
)
{
    float fontSize = ImGui::GetFontSize();
    int column;
    bool managed
    (
        uniform->managedType != ManagedType::None
    );
    if 
    (
        (managed && !showSharedAndDefaultUniforms) ||
        uniform->managedType == ManagedType::ResourceResolution
    )
        return false;
    bool typeChanged = false;
    auto& su = *(sharedUniforms);
    
    START_ROW(row, column)

    START_COLUMN(column) // Action column --------------------------------------
    float y0 = 0;
    if (!managed)
    {
        float halfButtonSize(1.7*fontSize);
        if (ImGui::Button(ICON_FA_TRASH, ImVec2(halfButtonSize, 0)))
        {
            updateLayersDueToUniformDeletion(uniform);
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
            // TODO Test this approach of updating sharedness status
            uniform->isSharedByUser = !uniform->isSharedByUser;
            deferredActionBuffer.add
            (
                [&uniform, layer, this]()
                {
                    if (uniform->isSharedByUser)
                        uniform->setOwner(&sharedUniforms->fragment);
                    else 
                        uniform->setOwner(&layer->fragment);
                    for (auto& l : layers)
                    {
                        l->compileShader();
                    }
                }
            );
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
    END_COLUMN(column)
    
    START_COLUMN(column) // Name column ----------------------------------------
    if (managed)
        ImGui::Text(uniform->name().c_str());
    else
    {
        if (ImGui::InputText("##uniformName", &uniform->name()))
        {
            uniform->markForSubmissionToAllClientBuffers();
            Helpers::enforceUniqueName
            (
                uniform->name(),
                layer->fragment.uniforms,
                uniform.get(),
                true
            );
            updateLayersDueToUniformTypeOrNameChanged
            (
                uniform, 
                false // Do NOT recompile shaders automatically on rename
            );
        }
    }
    bool named(uniform->name().size() > 0);
    if (showSeparator)
        ImGui::Separator();
    END_COLUMN(column)

    START_COLUMN(column) // Type column ----------------------------------------
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
        for(auto uniformTypeName : Uniform::supportedTypeNames)
        {
            if (!ImGui::Selectable(uniformTypeName.c_str()))
                continue;
            auto selectedType = 
                vir::Shader::uniformNameToType[uniformTypeName];
            if (selectedType == uniform->type())
                continue;
            typeChanged = true;
            uniform->setType(selectedType);
            updateLayersDueToUniformTypeOrNameChanged
            (
                uniform, 
                true // Recompile shaders automatically on type change
            );
        }
        ImGui::EndCombo();
    }
    if (showSeparator)
        ImGui::Separator();
    END_COLUMN(column)

    START_COLUMN(column) // Bounds column ------------------------------------------
    bool boundsChanged(false);
    glm::vec2& bounds = uniform->gui.bounds;
    if (uniform->gui.showBounds)
        boundsChanged = uniform->renderEditBoundsButtonGui(true);
    if (showSeparator)
    {
        if (y0 > 0)
            ImGui::SetCursorPosY(y0);
        ImGui::Separator();
    }
    END_COLUMN(column)

    START_COLUMN(column) // Value column ---------------------------------------
    switch(uniform->type())
    {
        case vir::Uniform::Type::Bool :
        {
            auto value = uniform->getValue<bool>();
            if (ImGui::Checkbox((value) ? "true" : "false", &value))
            {
                uniform->setValue(value, Type::Bool);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::UInt : //-------------------------
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int : //--------------------------
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int2 : //-------------------------
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int3 : //-------------------------
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int4 : //-------------------------
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Float : //------------------------
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
                uniform->managedType == 
                    Uniform::ManagedType::LayerAspectRatio
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Float2 : //-----------------------
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
                uniform->managedType == 
                    Uniform::ManagedType::LayerResolution || 
                uniform->managedType == 
                    Uniform::ManagedType::ResourceResolution 
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
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Float3 : //-----------------------
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
                    uniform->setValue(value, Type::Float3);
                    if (named)
                    {
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
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
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
                    }
                }
            }
            break;
        }
        case vir::Uniform::Type::Float4 : //-----------------------
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
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
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
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
                    }
                }
            }
            break;
        }

        case vir::Uniform::Type::Sampler2D :
        case vir::Uniform::Type::Image2D :
        {
            auto resource = 
                uniform->getValuePtr<Resource>();
            std::string name
            (
                (resource != nullptr) ? resource->name() : ""
            );
            if (ImGui::BeginCombo("##tx2DSelector", name.c_str()))
            {
                for (UPtr<Resource>& r : resources)
                {
                    if 
                    (
                        r->type() != Resource::Type::Texture2D &&
                        r->type() != Resource::Type::AnimatedTexture2D &&
                        r->type() != Resource::Type::Framebuffer
                    )
                        continue;
                    if (ImGui::Selectable(r->name().c_str()))
                        assignResourceToUniform(r, uniform);
                }
                ImGui::EndCombo();
            }
            break;
        }
        case vir::Uniform::Type::Sampler3D :
        case vir::Uniform::Type::Image3D :
        {
            auto resource = 
                uniform->getValuePtr<Resource>();
            std::string name
            (
                (resource != nullptr) ? resource->name() : ""
            );
            if (ImGui::BeginCombo("##tx3DSelector", name.c_str()))
            {
                for (auto& r : resources)
                {
                    if (r->type() != Resource::Type::Texture3D)
                        continue;
                    if (ImGui::Selectable(r->name().c_str()))
                        assignResourceToUniform(r, uniform);
                }
                ImGui::EndCombo();
            }
            break;
        }
        case vir::Uniform::Type::SamplerCube :
        case vir::Uniform::Type::ImageCube :
        {
            auto resource = 
                uniform->getValuePtr<Resource>();
            std::string name
            (
                (resource != nullptr) ? resource->name() : ""
            );
            if (ImGui::BeginCombo("##cmSelector", name.c_str()))
            {
                for (auto& r : resources)
                {
                    if (r->type() != Resource::Type::Cubemap)
                        continue;
                    if (ImGui::Selectable(r->name().c_str()))
                        assignResourceToUniform(r, uniform);
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
    END_COLUMN(column)
    
    END_ROW(row)

    return typeChanged;
    
}; // End of renderUniform

//----------------------------------------------------------------------------//
// Resources -----------------------------------------------------------------//
//----------------------------------------------------------------------------//

void App::renderResourcesMenuGui()
{
    if 
    (
        ImGui::SmallButton
        (
            Resource::gui.isDetachedFromControlPanel ? 
            ICON_FA_WINDOW_MAXIMIZE : 
            ICON_FA_ARROW_RIGHT
        )
    )
        Resource::gui.isDetachedFromControlPanel = 
            !Resource::gui.isDetachedFromControlPanel;
    ImGui::SameLine();
    if (!Resource::gui.isDetachedFromControlPanel)
    {
        if (ImGui::BeginMenu("Resource manager"))
        {
            Resource::gui.isOpen = true;
            renderResourcesTableGui();
            ImGui::EndMenu();
        }
        else
            Resource::gui.isOpen = false;
        return;
    }
    ImGui::MenuItem("Resource manager", NULL, &Resource::gui.isOpen);
}

//----------------------------------------------------------------------------//

void App::renderResourcesTableGui()
{
    if (!Resource::gui.isOpen)
        return;
    if (Resource::gui.isDetachedFromControlPanel)
    {
        ImGui::SetNextWindowSize(ImVec2(900,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Resource manager", &Resource::gui.isOpen, windowFlags);

        // Refresh icon if needed
        static bool isIconSet(false);
        static bool isWindowDocked(ImGui::IsWindowDocked());
        if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
        {
            isIconSet = vir::ImGuiRenderer::setWindowIcon
            (
                "Resource manager", 
                ByteData::Icon::sTIconData, 
                ByteData::Icon::sTIconSize,
                false
            );
            isWindowDocked = ImGui::IsWindowDocked();
        }
    }

    float cursorPosY0 = ImGui::GetCursorPosY();
    float fontSize = ImGui::GetFontSize();
    static float tableHeight = 0;
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##resourceTable", 6, flags, ImVec2(0., tableHeight)))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("##controls", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Type", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Preview", flags, 4.0*fontSize);
        ImGui::TableSetupColumn("Name", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Resolution", flags, 10.0*fontSize);
        ImGui::TableSetupColumn
        (
            "Aspect ratio", 
            flags, 
            Resource::gui.isDetachedFromControlPanel ? 
            ImGui::GetContentRegionAvail().x : 8.0*fontSize
        );
        ImGui::TableHeadersRow();

        const int nRows = resources.size();
        for (int row=0; row<nRows; row++)
        {
            renderResourcesTableRowGui(row);
        }
        renderAddResourceButtonGui(nRows);
        tableHeight = (ImGui::GetCursorPosY()-cursorPosY0);
        ImGui::EndTable();
    }

    if (Resource::gui.isDetachedFromControlPanel)
        ImGui::End();
}

//----------------------------------------------------------------------------//

void App::renderResourcesTableRowGui(int row)
{
    UPtr<Resource>& resource = resources[row];
    float fontSize = ImGui::GetFontSize();
    int column = 0;
    START_ROW(row, column)
    START_COLUMN(column) // Actions column -------------------------------------
    // The button returns true if any changes made require shade recompilation
    if (resource->renderActionsButtonGui(resources, deferredActionBuffer))
    {
        deferredActionBuffer.add
        (
            [this](){for (auto& layer : layers) layer->compileShader();}
        );
    }
    END_COLUMN(column)
    START_COLUMN(column) // Type column ----------------------------------------
    std::string typeName = Resource::typeToName.at(resource->type());
    if (auto texture = dynamic_cast<Texture2DResource*>(resource.get()))
    {
        // Small fix to distinguish storage textures from "regular" image-
        // loaded textures. I will need to revist the whole storage-texture
        // thing in the future, or add the same functionalities (namely
        // resizing/reformatting) to "regular" image-loaded textures to 
        // remove such somewhat artificial distinctions altogether
        if (!texture->hasRawData())
            typeName += " (S)";
    }
    ImGui::Text(typeName.c_str());
    END_COLUMN(column)
    START_COLUMN(column) // Preview column -------------------------------------
    float x = resource->width();
    float y = resource->height();
    float aspectRatio = x/y;
    auto previewTexture2D = 
    [&aspectRatio]
    (
        const Resource* resource, 
        float sideSize, 
        float offset
    )->void
    {
        ImVec2 previewSize;
        ImVec2 hoverSize{256,256};
        if (aspectRatio > 1.0)
        {
            previewSize = ImVec2(sideSize, sideSize/aspectRatio);
            hoverSize.y /= aspectRatio;
        }
        else
        { 
            previewSize = ImVec2(sideSize*aspectRatio, sideSize);
            hoverSize.x *= aspectRatio;
        }
        float startx = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(startx + offset);
#define SHOW_IMAGE(size)                                                \
ImGui::Image                                                            \
(                                                                       \
    (ImTextureID)                                                       \
    (                                                                   \
        resource->type() != Resource::Type::AnimatedTexture2D ?         \
        resource->id() :                                                \
        dynamic_cast<const AnimatedTexture2DResource*>(resource)->frameId()\
    ),                                                                  \
    size,                                                               \
    {0,1},                                                              \
    {1,0}                                                               \
);
        SHOW_IMAGE(previewSize)
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            SHOW_IMAGE(hoverSize)
            ImGui::EndTooltip();
        }
    };

    if (resource->isInternalFormatUnsigned())
        ImGui::Text("N/A");
    else if 
    (
        resource->type() == Resource::Type::Texture2D ||
        resource->type() == Resource::Type::AnimatedTexture2D ||
        resource->type() == Resource::Type::Framebuffer
    )
        previewTexture2D(resource.get(), 1.4*fontSize, 1.3*fontSize);
    else if (resource->type() == Resource::Type::Cubemap)
    {
        auto cubemap = (CubemapResource*)resource.get();
        for (int i=0; i<6; i++)
        {
            float offset = (i == 0 || i == 3) ? 0.75*fontSize : 0.0;
            previewTexture2D
            (
                cubemap->faces()[i].get(), 
                0.5*fontSize, 
                offset
            );
            if ((i+1)%3 != 0)
                ImGui::SameLine();
        }
    }
    END_COLUMN(column)
    START_COLUMN(column) // Name column ----------------------------------------
    if (resource->type() != Resource::Type::Framebuffer)
    {
        if (ImGui::InputText("##resourceName", resource->namePtr()))
        {
            Helpers::enforceUniqueName
            (
                *(resource->namePtr()),
                resources,
                resource.get()
            );
        }
    }
    else
        ImGui::Text(resource->name().c_str());
    END_COLUMN(column)
    START_COLUMN(column) // Resolution column ----------------------------------
    if (resource->type() == Resource::Type::Texture3D)
        ImGui::Text("%d x %d x %d", (int)x, (int)y, (int)resource->depth());
    else
        ImGui::Text("%d x %d", (int)x, (int)y);
    END_COLUMN(column)
    START_COLUMN(column) // Aspect ratio column --------------------------------
    ImGui::Text("%.3f", aspectRatio);
    END_COLUMN(column)
    END_ROW(row)
}

//----------------------------------------------------------------------------//

void App::renderAddResourceButtonGui(int row)
{
    int column = 0;
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1,0)))
        ImGui::OpenPopup("##addResourcePopup");
    if (ImGui::BeginPopup("##addResourcePopup"))
    {
        float buttonWidth = 12*ImGui::GetFontSize();
        renderLoadResourceFromFileButtonGui(buttonWidth);
        renderCreateTexture2DButtonGui(buttonWidth);
        renderCreateTexture3DButtonGui(buttonWidth);
        renderCreateAnimatedTexture2DButtonGui(buttonWidth);
        renderCreateCubemapButtonGui(buttonWidth);
        ImGui::EndPopup();
    }
    END_ROW(row)
}

//----------------------------------------------------------------------------//

void App::renderLoadResourceFromFileButtonGui(float width)
{
    if (ImGui::Button("Load from file", ImVec2(width, 0)))
    {
        Resource::fileDialog.runOpenFileDialog
        (
            "Select an image or GIF",
            {
                "Image files (.png,.jpg,.jpeg,.bmp,.gif)", 
                "*.png *.jpg *.jpeg *.bmp *.gif"
            },
            "."
        );
        deferredActionBuffer.add
        (
            [this]()
            {
                auto filepath = Resource::fileDialog.selection().front();
                bool isGif = Helpers::fileExtension(filepath) == ".gif";
                if (isGif)
                    resources.emplace_back
                    (
                        AnimatedTexture2DResource::create(filepath)
                    );
                else 
                    resources.emplace_back
                    (
                        Texture2DResource::create(filepath)
                    );
                std::string name = Helpers::filename(filepath);
                auto& resource = resources.back();
                Helpers::enforceUniqueName
                (
                    name, 
                    resources, 
                    resource.get()
                );
                resource->setName(name);
            },
            []() -> bool
            {
                return Resource::fileDialog.validSelection();
            }
        );
    }
}

//----------------------------------------------------------------------------//

void App::renderCreateTexture2DButtonGui(float width)
{
    static Texture2DResource::EditorGuiData editorGuiData = {};
    if (ImGui::Button("Create texture-2D", ImVec2(width, 0)))
    {
        ImGui::OpenPopup("##createTexture2DPopup");
        editorGuiData = {};
    }
    if (ImGui::BeginPopup("##createTexture2DPopup"))
    {
        ImGui::Text(
R"(Create a blank texture, useful for e.g., 
shader data storage via imageLoad and
imageStore operations. Data written to 
these textures will not be saved within 
the project)");
        ImGui::Separator();
        Texture2DResource::renderEditorGui(editorGuiData, 11);
        if (ImGui::Button("Create texture-2D", ImVec2(-1, 0)))
        {
            auto egd = editorGuiData;
            deferredActionBuffer.add
            (
                [this, egd]()
                {
                    resources.emplace_back
                    (
                        Texture2DResource::create
                        (
                            egd.resolution.x,
                            egd.resolution.y,
                            egd.internalFormat
                        )
                    );
                }
            );
        }
        Helpers::renderTextureMemoryEstimateGui
        (
            editorGuiData.nPixels(),
            editorGuiData.internalFormat,
            true
        );
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void App::renderCreateTexture3DButtonGui(float width)
{
    static Texture3DResource::EditorGuiData editorGuiData = {};
    if (ImGui::Button("Create texture-3D", ImVec2(width, 0)))
    {
        ImGui::OpenPopup("##createTexture3DPopup");
        editorGuiData = {};
    }
    if (ImGui::BeginPopup("##createTexture3DPopup"))
    {
        ImGui::Text(
R"(Create a blank texture, useful for e.g., 
shader data storage via imageLoad and
imageStore operations. Data written to 
these textures will not be saved within 
the project)");
        ImGui::Separator();
        Texture3DResource::renderEditorGui(editorGuiData, 11);
        if (ImGui::Button("Create texture-3D", ImVec2(-1, 0)))
        {
            auto egd = editorGuiData;
            deferredActionBuffer.add
            (
                [this, egd]()
                {
                    resources.emplace_back
                    (
                        Texture3DResource::create
                        (
                            egd.resolution.x,
                            egd.resolution.y,
                            egd.resolution.z,
                            egd.internalFormat
                        )
                    );
                }
            );
        }
        Helpers::renderTextureMemoryEstimateGui
        (
            editorGuiData.nPixels(),
            editorGuiData.internalFormat,
            true
        );
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void App::renderCreateAnimatedTexture2DButtonGui(float width)
{
    static AnimatedTexture2DResource::EditorGuiData editorGuiData = {};
    if (ImGui::Button("Create animation", ImVec2(width, 0)))
    {
        ImGui::OpenPopup("##createAnimationPopup");
        editorGuiData.frameResolution = {0u, 0u};
    }   
    if (ImGui::BeginPopup("##createAnimationPopup"))
    {
        if 
        (
            AnimatedTexture2DResource::renderEditorButtonGui
            (
                editorGuiData, 
                resources, 
                "Create"
            )
        )
        {
            auto* egd = &editorGuiData;
            deferredActionBuffer.add
            (
                [this, egd]()
                {
                    resources.emplace_back
                    (
                        AnimatedTexture2DResource::create
                        (
                            egd->unmanagedFrames
                        )
                    );
                    egd->unmanagedFrames.clear();
                    egd->orderedUnmanagedFrameNames.clear();
                }
            );
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void App::renderCreateCubemapButtonGui(float width)
{
    static CubemapResource::EditorGuiData editorGuiData = {};
    if (ImGui::Button("Create cubemap", ImVec2(width, 0)))
    {
        ImGui::OpenPopup("##createCubemapPopup");
        editorGuiData.faceResolution = {0, 0};
        for (int i=0;i<6;i++)
            editorGuiData.selectedTextureResources[i] = 
                WPtr<Texture2DResource>();
    }   
    if (ImGui::BeginPopup("##createCubemapPopup"))
    {
        if 
        (
            CubemapResource::renderEditorButtonGui
            (
                editorGuiData, 
                resources, 
                "Create"
            )
        )
        {
            auto egd = editorGuiData;
            deferredActionBuffer.add
            (
                [this, egd]()
                {
                    resources.emplace_back
                    (
                        CubemapResource::create
                        (
                            egd.selectedTextureResources
                        )
                    );
                }
            );
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void App::renderExportMenuGui()
{
    auto& es = exportState;
    float fontSize(ImGui::GetFontSize());
    ImGui::Text("Export type                 ");
    ImGui::SameLine();

    static std::map<ExportType, const char*> exportTypeToName = {
        {ExportType::Image, "Image"},
        {ExportType::GIF, "GIF"},
        {ExportType::VideoFrames, "Video frames"}
    };

    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::BeginCombo
        (
            "##exportTypeSelector", 
            exportTypeToName[es.type]
        )
    )
    {
        for(auto e : exportTypeToName)
        {
            bool disabled
            (
                e.first == ExportType::GIF && 
                !es.gifEncoder->canRunOnDeviceInUse()
            );
            if (disabled)
                ImGui::BeginDisabled();
            if 
            (
                ImGui::Selectable
                (
                    disabled ? 
                    "GIF (requires OpenGL v4.3 or above)" :
                    e.second
                )
            )
            {
                es.type = e.first;
                // Ensure valid GIF fps (can only be such that the resulting
                // frame duration is an integer number of hundreds of a second)
                if (es.type == ExportType::GIF)
                    es.fps = 
                        100.f/int(100.f/std::min(es.fps,100.f)+.5f);
            }
            if (disabled)
                ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (es.type != ExportType::Image)
    {
        ImGui::Text("Start time                  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat("##exporterStartTime", &es.startTime);
        ImGui::PopItemWidth();
        ImGui::Text("End time                    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat("##exporterEndTime", &es.endTime);
        ImGui::PopItemWidth();
        ImGui::Text("Target FPS                  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        bool fpsChanged = ImGui::InputFloat
        (
            "##exporterFps", 
            &es.fps, 
            0.0f, 
            0.0f, 
            "%.2f"
        );
        ImGui::PopItemWidth();
        
        ImGui::PushItemWidth(-1);
        if (es.type == ExportType::GIF)
        {
            // Ensure valid GIF fps (can only be such that the resulting frame
            // duration is an integer number of hundreds of a second)
            if (fpsChanged)
                es.fps = 
                    100.f/int(100.f/std::min(es.fps,100.f)+.5f);
            
            ImGui::Text("Palette bit depth           ");
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text(
R"(Determines the GIF palette size. The number of colors in the palette is 
2^(palette bit depth))");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            ImGui::SliderInt
            (
                "##exporterGifPaletteBitDepth", 
                (int*)&es.gifPaletteBitDepth, 
                1, 
                8
            );
            ImGui::PopItemWidth();

            ImGui::Text("Palette mode                ");
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::BeginCombo
                (
                    "##paletteModeSelector", 
                    vir::GifEncoder::paletteModeToName[es.gifPaletteMode]
                )
            )
            {
                for(auto e : vir::GifEncoder::paletteModeToName)
                {
                    if (ImGui::Selectable(e.second))
                        es.gifPaletteMode = e.first;
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            
            ImGui::Text("Transparency cutoff         ");
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text(
R"(Alpha channel threshold below which (threshold excluded) a pixel is considered
as fully transparent. If set to 0, the output GIF file is fully opaque with no
transparency)");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            ImGui::SliderInt
            (
                "##exportToolAlphaCutoff", 
                (int*)&es.gifAlphaCutoff, 
                0, 
                255
            );
            ImGui::PopItemWidth();
            ImGui::Text("Color dithering             ");
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::BeginCombo
                (
                    "##gifDitheringSelector", 
                    vir::Quantizer::Settings::ditherModeToName.at
                    (
                        es.gifDitherMode
                    ).c_str()
                )
            )
            {
                for(auto e : vir::Quantizer::Settings::ditherModeToName)
                {
                    if (ImGui::Selectable(e.second.c_str()))
                    {
                        es.gifDitherMode = e.first;
                        break;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
    }
    if (es.type != ExportType::Image)
        ImGui::Text("Render passes per frame     ");
    else
        ImGui::Text("Render passes               ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    ImGui::InputInt("##nRenderPasses", (int*)&es.nRenderPasses);
    es.nRenderPasses = 
        std::min(std::max(1u, es.nRenderPasses), 32768u);
    ImGui::PopItemWidth();
    if (es.nRenderPasses > 1 && es.type != ExportType::Image)
    {
        ImGui::Text("Multi-pass first frame only ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##exportAreRenderPassesOnFirstFrameOnly", 
            &es.areRenderPassesOnFirstFrameOnly
        );
    }

    ImGui::Text("Reset iFrame after export   ");
    ImGui::SameLine();
    ImGui::Checkbox
    (
        "##resetFrameCounterAfterExport", 
        &es.resetFrameCounterAfterExport
    );

    //--------------------------------------------------------------------------

    auto window = vir::Window::instance();

    ImGui::Separator();
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##exportResolutionTable", 3, flags))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("Layer name", flags, 9.f*fontSize);
        ImGui::TableSetupColumn("Export resolution", flags, 9.25f*fontSize);
        ImGui::TableSetupColumn("Clear policy", flags, 9.5f*fontSize);
        ImGui::TableHeadersRow();

        glm::ivec2 outputResolution = es.outputResolution;
        float& outputScale = es.outputResolutionScale;
        bool& outputResolutionChanged = es.cache.outputResolutionChanged;
        
        // Actual table render -------------------------------------------------
        int nRows = (int)layers.size();
        for (int row = 0; row < nRows+1; row++)
        {
            int col = 0;
            
            ImGui::PushID(row);
            ImGui::TableNextRow(0, 1.6*fontSize);
            
            if (row == 0) //
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::Text("Output image");
                
                ImGui::TableSetColumnIndex(col++);
                ImGui::PushItemWidth(6.75*fontSize);
                glm::ivec2 outputResolution0 = outputResolution;
                ImGui::InputInt2
                (
                    "##exportResolution", 
                    glm::value_ptr(outputResolution)
                );  
                if (outputResolution0.x != outputResolution.x)
                {
                    outputResolution.y = 
                        std::max(outputResolution.x/window->aspectRatio(), 1.f);
                    outputResolutionChanged = true;
                }
                else if (outputResolution0.y != outputResolution.y)
                {
                    outputResolution.x = 
                        std::max(outputResolution.y*window->aspectRatio(), 1.f);
                    outputResolutionChanged = true;
                }
                outputResolution.x=std::max(outputResolution.x, 1);
                outputResolution.y=std::max(outputResolution.y, 1);
                if (outputResolutionChanged)
                {
                    outputScale = 
                        outputResolution.x > outputResolution.y ?
                        (float)outputResolution.x/window->width() :
                        (float)outputResolution.y/window->height();
                }
                es.outputResolution = outputResolution;
                ImGui::PopItemWidth();
                ImGui::TableSetColumnIndex(col++);
                ImGui::PopID();
                continue;
            }
            
            auto& layer = layers[row-1];
            bool layerRendersToWindow
            (
                layer->renderingTarget() == Layer::RenderState::Target::Window
            );
            auto& les = layer->exportSettings;
            auto& layerClearPolicy = les.clearPolicy;
            auto& layerWindowResolutionScale = les.windowResolutionScale;
            auto& layerExportResolution = les.resolution;
            auto& layerExportScale = les.resolutionScale;
            auto& layerRescale = les.rescaleWithOutput;

            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            ImGui::Text(layer->name().c_str());
            ImGui::PopItemWidth();

            // Column 1 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(6.75*fontSize);
            if (outputResolutionChanged)
            {
                layerWindowResolutionScale = outputScale;
                if (layerRescale)
                    layerExportResolution = 
                        layer->resolution()*
                        layerExportScale*outputScale + .5f;
            }
            else if (!layerRendersToWindow)
            {
                glm::ivec2 layerExportResolution0 = layerExportResolution;
                bool layerExportResolutionChanged = 
                    ImGui::InputInt2
                    (
                        "##exporterLayerResolution", 
                        glm::value_ptr(layerExportResolution)
                    );
                if (layerExportResolution0.x != layerExportResolution.x)
                {
                    layerExportResolution.y = 
                        std::max
                        (
                            layerExportResolution.x/layer->aspectRatio(),
                            1.f
                        );
                }
                else if (layerExportResolution0.y != layerExportResolution.y)
                    layerExportResolution.x = 
                        std::max
                        (
                            layerExportResolution.y*layer->aspectRatio(),
                            1.f
                        );
                if (layerExportResolutionChanged)
                {
                    layerExportScale =
                        layerExportResolution.x > layerExportResolution.y ?
                        (float)layerExportResolution.x/outputScale/
                            layer->resolution().x :
                        (float)layerExportResolution.y/outputScale/
                            layer->resolution().y;
                }
            }
            else
            {
                layerExportResolution = outputResolution;
                ImGui::BeginDisabled();
                ImGui::InputInt2
                (
                    "##exporterResolution", 
                    glm::value_ptr(layerExportResolution)
                );
                ImGui::EndDisabled();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            bool layerRescale0(layerRescale);
            if (layerRendersToWindow)
                ImGui::BeginDisabled();
            if 
            (
                ImGui::Button
                (
                    layerRescale ? 
                    ICON_FA_LOCK : 
                    ICON_FA_LOCK_OPEN, 
                    {-1,0}
                )
            )
                layerRescale = !layerRescale;
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
                ImGui::BeginTooltip()
            )
            {
                if (layerRescale)
                    ImGui::Text(
ICON_FA_LOCK " - The layer resolution will rescale\nwith the output resolution"
                    );
                else
                    ImGui::Text(
ICON_FA_LOCK_OPEN " - The layer resolution will not rescale\nwith the output "
"resolution"
                    );
                ImGui::EndTooltip();
            }
            if (layerRendersToWindow)
                ImGui::EndDisabled();
            if (layerRescale && !layerRescale0)
            {
                layerExportScale = 1;
                layerExportResolution = 
                    (glm::vec2)layer->resolution()*
                    layerExportScale*outputScale + .5f;
            }
            ImGui::PopItemWidth();

            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            if (!layerRendersToWindow)
            {
                static std::map
                <
                    Layer::ExportSettings::FramebufferClearPolicy, 
                    std::string
                > frameBufferClearPolicyToName
                {
                    {
                        Layer::ExportSettings::FramebufferClearPolicy::
                        ClearOnEveryFrameExport, 
                        "On every frame"
                    },
                    {
                        Layer::ExportSettings::FramebufferClearPolicy::
                        ClearOnFirstFrameExport, 
                        "On first frame"
                    },
                    {Layer::ExportSettings::FramebufferClearPolicy::
                        None, 
                        "None"
                    }
                };
                auto selectedExportClearPolicy = 
                        frameBufferClearPolicyToName.at(layerClearPolicy);
                ImGui::PushItemWidth(-1);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##layerExportClearPolicy",
                        selectedExportClearPolicy.c_str()
                    )
                )
                {
                    for (auto entry : frameBufferClearPolicyToName)
                    {
                        if (ImGui::Selectable(entry.second.c_str()))
                            layerClearPolicy = entry.first;
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }
            
            ImGui::PopID();

            layerExportResolution.x=std::max(layerExportResolution.x, 1);
            layerExportResolution.y=std::max(layerExportResolution.y, 1);
        }
        outputResolutionChanged = false;
        ImGui::EndTable();
    }

    //--------------------------------------------------------------------------

    ImGui::Separator();

    bool disableExportButton = es.isActive || renderState.isPaused;
    if (disableExportButton)
        ImGui::BeginDisabled();
    if 
    (
        ImGui::Button
        (
            es.isActive ? 
            (
                es.gifPaletteMode == PaletteMode::StaticAveraged && 
                !es.isAveragedPaletteReady ?
                "Computing averaged palette..." :
                "Exporting..." 
            ) : 
            "Export", 
            {-1,0}
        ))
    {
        std::vector<std::string> filters;
        if (es.type == ExportType::GIF)
            filters = {"Image files (*.gif)", "*.gif"};
        else
            filters = {"Image files (*.png)", "*.png"};
        fileDialog.runSaveFileDialog("Export graphics to file", filters);

        // Will begin export once the output filepath for the export is provided
        deferredActionBuffer.add
        (
            [this]()
            {
                beginExport();
            },
            [this]()
            {
                return fileDialog.validSelection();
            }
        );
    }
    if (disableExportButton)
        ImGui::EndDisabled();

    if (es.isActive)
    {   
        float progress = 
            (
                (float)es.frameIndex +
                (float)renderState.passIndex/es.nRenderPasses
            )/std::max(es.nFrames, 1u);
        float rate = 
            1./
            es.nFrames/
            es.nRenderPasses/
            std::max
            (
                vir::Window::instance()->time()->smoothOuterTimestep(), 
                1e-4f
            );
        std::string text = 
            Helpers::format(100.f*progress, 1)+" % | "+
            Helpers::format((1.-progress)/rate, 1)+" s left";
        ImGui::ProgressBar
        (
            progress, 
            ImVec2(-1, 0.0f),
            text.c_str()
        );
    }
}

}