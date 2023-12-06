/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "resources/resourcemanager.h"
#include "tools/exporttool.h"
#include "tools/quantizationtool.h"
#include "tools/findreplacetexttool.h"
#include "data/coderepository.h"
#include "data/data.h"
#include "data/about.h"
#include "misc/misc.h"

#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

#define IN_MENU_ENTRY(cmpt, cmptName)                                       \
if (ImGui::BeginMenu(cmptName))                                             \
{                                                                           \
    *(cmpt->isGuiOpenPtr()) = true;                                         \
    cmpt->renderGui();                                                      \
    ImGui::EndMenu();                                                       \
}                                                                           \
else                                                                        \
    *(cmpt->isGuiOpenPtr()) = false;                                        \


#define OZ_MENU_ENTRY(cmpt, cmptName)                                       \
if(ImGui::SmallButton(cmpt->isGuiInMenu() ? "O" : "Z" ))                    \
    cmpt->toggleIsGuiInMenu();                                              \
ImGui::SameLine();                                                          \
if (cmpt->isGuiInMenu())                                                    \
{                                                                           \
    if (ImGui::BeginMenu(cmptName))                                         \
    {                                                                       \
        *(cmpt->isGuiOpenPtr()) = true;                                     \
        cmpt->renderGui();                                                  \
        ImGui::EndMenu();                                                   \
    }                                                                       \
    else                                                                    \
        *(cmpt->isGuiOpenPtr()) = false;                                    \
}                                                                           \
else if(ImGui::MenuItem(cmptName, NULL, cmpt->isGuiOpenPtr())){}

#define OZ_MENU_ENTRY_DISABLEABLE(cmpt, cmptName, disable, disableText)     \
if (disable) ImGui::BeginDisabled();                                        \
if(ImGui::SmallButton(cmpt->isGuiInMenu() ? "O" : "Z" ))                    \
    cmpt->toggleIsGuiInMenu();                                              \
ImGui::SameLine();                                                          \
if (cmpt->isGuiInMenu())                                                    \
{                                                                           \
    if (ImGui::BeginMenu(cmptName))                                         \
    {                                                                       \
        if (disable)                                                        \
        {                                                                   \
            ImGui::PushTextWrapPos(40.0f*fontSize);                         \
            ImGui::Text(disableText);                                       \
            ImGui::PopTextWrapPos();                                        \
        }                                                                   \
        else                                                                \
        {                                                                   \
            *(cmpt->isGuiOpenPtr()) = true;                                 \
            cmpt->renderGui();                                              \
        }                                                                   \
        ImGui::EndMenu();                                                   \
    }                                                                       \
    else                                                                    \
        *(cmpt->isGuiOpenPtr()) = false;                                    \
}                                                                           \
else if(ImGui::MenuItem(cmptName, NULL, cmpt->isGuiOpenPtr())){}            \
if (disable) ImGui::EndDisabled(); 

void ShaderThingApp::updateGui()
{
    // Disable reading/writing from/to imgui.ini
    static ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigDockingTransparentPayload = true;
    
    // Load ImGui font
    static bool fontLoaded(false);
    static ImFont* font = nullptr;
    static ImFontConfig config; 
    if (!fontLoaded)
    {
        float baseFontSize = 26.f;
        config.PixelSnapH = true;
        config.OversampleV = 3.0;
        config.OversampleH = 3.0;
        config.RasterizerMultiply = 1.0;
        // The 26-36.5 ratio between Western writing systems' characters and
        // asian logograms/characters is set so that the latter are (almost)
        // exactly twice as wide as the former, for readability, valid for the
        // selected fonts at hand
        font = io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)FontData::CousineRegularData,
            FontData::CousineRegularSize, 
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );
        config.MergeMode = true;
        config.RasterizerMultiply = 1.25;
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)FontData::CousineRegularData, 
            FontData::CousineRegularSize,
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesCyrillic()
        );
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)FontData::CousineRegularData, 
            FontData::CousineRegularSize,
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesGreek()
        );

        // Font icons from FontAwesome5 (free)
        float iconFontSize = baseFontSize*2.f/3.f;
        static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig iconConfig; 
        iconConfig.MergeMode = true; 
        iconConfig.PixelSnapH = true; 
        iconConfig.GlyphMinAdvanceX = iconFontSize;
        io.Fonts->AddFontFromMemoryCompressedTTF
        ( 
            (void*)FontData::FontAwesome5FreeSolid900Data, 
            FontData::FontAwesome5FreeSolid900Size, 
            iconFontSize,
            &iconConfig, 
            iconRanges
        );

        io.Fonts->Build();
        fontLoaded = true;
        font->Scale = 0.6;
        fontScale_ = &font->Scale;
    }

#define CHECK_BUILD_CHARACTER_SET(name, data, dataSize, size)               \
static bool name##SetBuilt(false);                                          \
static bool name##SetBuilt0(false);                                         \
if (name##SetBuilt && name##SetBuilt != name##SetBuilt0){                   \
    io.Fonts->AddFontFromMemoryCompressedTTF(                               \
        (void*)data, dataSize, size, &config,                               \
        io.Fonts->GetGlyphRanges##name());                                  \
    io.Fonts->Build();                                                      \
    vir::ImGuiRenderer::destroyDeviceObjects();}                            \
name##SetBuilt0 = name##SetBuilt;
    
    CHECK_BUILD_CHARACTER_SET(
        Japanese, 
        FontData::DroidSansFallbackData, 
        FontData::DroidSansFallbackSize, 
        36.5)
    CHECK_BUILD_CHARACTER_SET(
        ChineseSimplifiedCommon, 
        FontData::DroidSansFallbackData, 
        FontData::DroidSansFallbackSize, 
        36.5)

    // Frame beginning, all other subcomponents' renderGui functions are to be
    // called after this point
    vir::ImGuiRenderer::newFrame(); // -----------------------------------------
    
    ImGui::SetNextWindowSize(ImVec2(750,900), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin("Control panel", NULL, flags);
    static bool setIcon(false);
    if (!setIcon)
    {
        setIcon = vir::ImGuiRenderer::setWindowIcon
        (
            "Control panel", 
            IconData::sTIconData, 
            IconData::sTIconSize,
            false
        );
    }
    float fontSize = ImGui::GetFontSize();

    // Check status of Ctrl+X key presses
    if (Misc::isCtrlKeyPressed(ImGuiKey_N)) // New
        stateFlags_[ST_NEW_PROJECT_CONFIRMATION_PENDING] = true;
    else if (Misc::isCtrlKeyPressed(ImGuiKey_O)) // Open
        stateFlags_[ST_OPEN_LOAD_DIALOG] = true;
    else if (Misc::isCtrlShiftKeyPressed(ImGuiKey_S)) // Save as
        stateFlags_[ST_OPEN_SAVE_DIALOG] = true;
    else if (Misc::isCtrlKeyPressed(ImGuiKey_S)) // Save
    {
        if (projectFilepath_ == "")
            stateFlags_[ST_OPEN_SAVE_DIALOG] = true;
        else 
            stateFlags_[ST_SAVE_PROJECT] = true;
    }
    else if (Misc::isCtrlKeyPressed(ImGuiKey_R)) // Restart rendering
        restartRendering();


    // Menu bar ----------------------------------------------------------------
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New project", "Ctrl+N"))
                stateFlags_[ST_NEW_PROJECT_CONFIRMATION_PENDING] = true;
            if (ImGui::MenuItem("Load project", "Ctrl+O"))
                stateFlags_[ST_OPEN_LOAD_DIALOG] = true;
            if (ImGui::MenuItem("Save project", "Ctrl+S"))
            {
                if (projectFilepath_ == "")
                    stateFlags_[ST_OPEN_SAVE_DIALOG] = true;
                else 
                    stateFlags_[ST_SAVE_PROJECT] = true;
            }
            if (ImGui::MenuItem("Save project as", "Ctrl+Shift+S"))
                stateFlags_[ST_OPEN_SAVE_DIALOG] = true;
            ImGui::Separator();
            OZ_MENU_ENTRY(exportTool_, "Export")
            ImGui::EndMenu();
            
        }
        if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::Text("Resolution");
                ImGui::SameLine();
                ImGui::PushItemWidth(8.0*fontSize);
                glm::ivec2 resolution = resolution_;
                glm::ivec2 resolution0 = resolution_;
                if 
                (
                    ImGui::InputInt2
                    (
                        "##windowResolution", 
                        glm::value_ptr(resolution)
                    )
                )
                {
                    Misc::limitWindowResolution(resolution);
                    if (resolution0 != resolution)
                    {
                        auto window = vir::GlobalPtr<vir::Window>::instance();
                        window->setSize(resolution.x, resolution.y, false);
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }
            for (auto layer : layersRef())
            {
                std::string layerSettingsName = "Layer ["+layer->name()+"]";
                if (ImGui::BeginMenu(layerSettingsName.c_str()))
                {
                    layer->renderGuiSettings();
                    ImGui::EndMenu();
                }
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Font"))
            {
                ImGui::Text("Scale");
                ImGui::SameLine();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat
                (
                    "##fontScale",
                    fontScale_,
                    0.005f,
                    0.5f,
                    1.0f,
                    "%.1f"
                );
                ImGui::PopItemWidth();
                ImGui::Separator();
                if (ImGui::BeginMenu("Load character sets"))
                {
                    static bool alwaysBuiltSet(true);
                    
                    ImGui::Text("Latin    ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##loadLatin", &alwaysBuiltSet);
                    ImGui::EndDisabled();

                    ImGui::Text("Cyrillic ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##loadCyrillic", &alwaysBuiltSet);
                    ImGui::EndDisabled();

                    ImGui::Text("Greek    ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##loadGreek", &alwaysBuiltSet);
                    ImGui::EndDisabled();

                    ImGui::Text("Japanese ");
                    ImGui::SameLine();
                    if (JapaneseSetBuilt) 
                        ImGui::BeginDisabled();
                    if (ImGui::Checkbox("##loadJapanese", &JapaneseSetBuilt))
                        JapaneseSetBuilt = true;
                    else if (JapaneseSetBuilt)
                        ImGui::EndDisabled();
                    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                    {
                        ImGui::Text(
"Warning: loading Japanese characters can take several hundreds of MB of RAM,\n"
"depending on your system. They can only be unloaded by exiting the program"
                        );
                        ImGui::EndTooltip();
                    }
                    
                    ImGui::Text("Chinese  ");
                    ImGui::SameLine();
                    if (ChineseSimplifiedCommonSetBuilt) 
                        ImGui::BeginDisabled();
                    if 
                    (
                        ImGui::Checkbox
                        (
                            "##loadChineseFull", 
                            &ChineseSimplifiedCommonSetBuilt
                        )
                    )
                        ChineseSimplifiedCommonSetBuilt = true;
                    else if (ChineseSimplifiedCommonSetBuilt) 
                        ImGui::EndDisabled();
                    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                    {
                        ImGui::Text("Simplified Chinese only");
                        ImGui::Separator();
                        ImGui::Text(
"Warning: loading Chinese characters can take several hundreds of MB of RAM,\n"
"depending on your system. They can only be unloaded by exiting the program"
                        );
                        ImGui::EndTooltip();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Misc"))
            {
                ImGui::Text("Time reset on rendering restart");
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(
R"(If true, the iTime uniform shared by all layers will be reset to 0 every time
the rendering is restarted, i.e., via: 1) shader recompilation (Ctrl+B) or 2) 
resetting the iFrame uniform (Ctrl+R))"
                    );
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::Checkbox
                (
                    "##timeResetOnRenderingRestart",
                    &stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART]
                );
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            OZ_MENU_ENTRY(resourceManager_, "Resource manager")
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Effects"))
        {
            OZ_MENU_ENTRY_DISABLEABLE
            (
                quantizationTool_, 
                "Quantizer", 
                !quantizationTool_->canRunOnDeviceInUse(),
                quantizationTool_->errorMessage().c_str()
            )
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Find"))
        {
            findReplaceTextTool_->renderGuiMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            IN_MENU_ENTRY(codeRepository_, "Code repository")
            IN_MENU_ENTRY(about_, "About ShaderThing")
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    renderGuiNewProject();
    renderGuiLoadProject();
    renderGuiSaveProject();
    layerManager_->renderGui();
    if (!resourceManager_->isGuiInMenu())
        resourceManager_->renderGui();
    if (!quantizationTool_->isGuiInMenu())
        quantizationTool_->renderGui();
    if (!exportTool_->isGuiInMenu())
        exportTool_->renderGui();

    ImGui::End();
    ImGui::ShowDemoWindow();
    vir::ImGuiRenderer::render();
}

//----------------------------------------------------------------------------//

void ShaderThingApp::renderGuiNewProject()
{
    bool& confirmationPending(stateFlags_[ST_NEW_PROJECT_CONFIRMATION_PENDING]);
    std::string text("New project");
    if (confirmationPending)
        ImGui::OpenPopup(text.c_str());
    if 
    (
        ImGui::BeginPopupModal
        (
            text.c_str(), 
            nullptr, 
            ImGuiWindowFlags_AlwaysAutoResize
        )
    )
    {
        ImGui::Text(
R"(Are you sure you want to start a new project?
Any unsaved changes to the current project will be lost!)");
        if (ImGui::Button("Confirm"))
        {
            stateFlags_[ST_NEW_PROJECT] = true;
            confirmationPending = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            confirmationPending = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void ShaderThingApp::renderGuiSaveProject()
{
    auto isGuiOpenPtr = &stateFlags_[ST_OPEN_SAVE_DIALOG];
    static std::string lastOpenedPath(".");
    if (*isGuiOpenPtr && !ImGuiFileDialog::Instance()->IsOpened())
    {
        ImGui::SetNextWindowSize(ImVec2(800,400), ImGuiCond_FirstUseEver);
        ImGuiFileDialog::Instance()->OpenDialog
        (
            "SaveAsFileDialog", 
            ICON_FA_FILE " Save as", 
            ".stf", 
            lastOpenedPath
        );
    }
    if (ImGuiFileDialog::Instance()->Display("SaveAsFileDialog")) 
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            projectFilepath_=ImGuiFileDialog::Instance()->GetFilePathName();
            projectFilename_=ImGuiFileDialog::Instance()->GetCurrentFileName();
            lastOpenedPath=ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
        }
        stateFlags_[ST_SAVE_PROJECT] = true;
        ImGuiFileDialog::Instance()->Close();
        *isGuiOpenPtr = false;
    }
}

//----------------------------------------------------------------------------//

void ShaderThingApp::renderGuiLoadProject()
{
    auto isGuiOpenPtr = &stateFlags_[ST_OPEN_LOAD_DIALOG];
    static std::string lastOpenedPath(".");
    if (*isGuiOpenPtr && !ImGuiFileDialog::Instance()->IsOpened())
    {
        ImGui::SetNextWindowSize(ImVec2(800,400), ImGuiCond_FirstUseEver);
        ImGuiFileDialog::Instance()->OpenDialog
        (
            "LoadFileDialog", 
            ICON_FA_FILE " Load project", 
            ".stf", 
            lastOpenedPath
        );
    }
    if (ImGuiFileDialog::Instance()->Display("LoadFileDialog"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            projectFilepath_=ImGuiFileDialog::Instance()->GetFilePathName();
            projectFilename_=ImGuiFileDialog::Instance()->GetCurrentFileName();
            lastOpenedPath=ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
            layerManager_->preLoadAdjustment();
            stateFlags_[ST_LOAD_PROJECT] = true;
        }
        ImGui::SetTooltip("Compiling project shaders...");
        ImGuiFileDialog::Instance()->Close();
        *isGuiOpenPtr = false;
    }
}

}