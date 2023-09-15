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
#include "thirdparty/imgui/backends/imgui_impl_glfw.h"
#include "thirdparty/imgui/backends/imgui_impl_opengl3.h"

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


#define OZ_MENU_ENTRY(cmpt, cmptName)                                     \
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
            26,
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );
        config.MergeMode = true;
        config.RasterizerMultiply = 1.25;
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)FontData::CousineRegularData, 
            FontData::CousineRegularSize,
            26,
            &config,
            io.Fonts->GetGlyphRangesCyrillic()
        );
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)FontData::CousineRegularData, 
            FontData::CousineRegularSize,
            26,
            &config,
            io.Fonts->GetGlyphRangesGreek()
        );
        /*io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)FontData::DroidSansFallbackData, 
            FontData::DroidSansFallbackSize,
            36.5,
            &config,
            io.Fonts->GetGlyphRangesChineseFull()
        );*/
        io.Fonts->Build();
        fontLoaded = true;
        font->Scale = 0.6;
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
    if (Misc::isCtrlKeyPressed(ImGuiKey_O)) // Open
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

    // Menu bar ----------------------------------------------------------------
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New project"))
                stateFlags_[ST_NEW_PROJECT] = true;
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
                if (ImGui::InputInt2("##resInput", glm::value_ptr(resolution)))
                {
                    if (resolution0 != resolution)
                    {
                        auto window = vir::GlobalPtr<vir::Window>::instance();
                        window->setSize(resolution.x, resolution.y);
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
                    &font->Scale, 
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
                    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                    {
                        ImGui::Text(
"Warning: loading Japanese characters can take several hundreds of MB of RAM,\n"
"depending on your system. They can only be unloaded by exiting the program"
                        );
                        ImGui::EndTooltip();
                    }
                    ImGui::SameLine();
                    if (JapaneseSetBuilt) 
                        ImGui::BeginDisabled();
                    if (ImGui::Checkbox("##loadJapanese", &JapaneseSetBuilt))
                        JapaneseSetBuilt = true;
                    else if (JapaneseSetBuilt) 
                        ImGui::EndDisabled();
                    
                    ImGui::Text("Chinese  ");
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
                    ImGui::EndMenu();
                }
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
            OZ_MENU_ENTRY(quantizationTool_, "Quantizer")
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

    renderGuiLoadProject();
    renderGuiSaveProject();
    layerManager_->renderGui();
    if (!resourceManager_->isGuiInMenu())
        resourceManager_->renderGui();
    if (!quantizationTool_->isGuiInMenu())
        quantizationTool_->renderGui();
    if (!exportTool_->isGuiInMenu())
        exportTool_->renderGui();

    /*if (exportTool_->isExporting() && ImGui::BeginTooltip())
    {
        ImGui::Text("Exporting...");
        ImGui::ProgressBar
        (
            exportTool_->exportProgress(), 
            ImVec2(fontSize*25, 0.0f)
        );
        ImGui::EndTooltip();
    }*/

    ImGui::End();
    //ImGui::ShowDemoWindow();
    vir::ImGuiRenderer::render();
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
            "Save as", 
            ".stf", 
            lastOpenedPath
        );
    }
    if (ImGuiFileDialog::Instance()->Display("SaveAsFileDialog")) 
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            projectFilepath_ = ImGuiFileDialog::Instance()->GetFilePathName();
            projectFilename_ = ImGuiFileDialog::Instance()->GetCurrentFileName();
            lastOpenedPath = ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
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
            "Load project", 
            ".stf", 
            lastOpenedPath
        );
    }
    if (ImGuiFileDialog::Instance()->Display("LoadFileDialog")) 
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            projectFilepath_ = ImGuiFileDialog::Instance()->GetFilePathName();
            projectFilename_ = ImGuiFileDialog::Instance()->GetCurrentFileName();
            lastOpenedPath = ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
            layerManager_->preLoadAdjustment();
            stateFlags_[ST_LOAD_PROJECT] = true;
        }
        ImGuiFileDialog::Instance()->Close();
        *isGuiOpenPtr = false;
    }
}

}