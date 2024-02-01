
#include "shaderthing-p/include/modules/frontend.h"
#include "shaderthing-p/include/modules/backend.h"
#include "shaderthing-p/include/modules/helpers.h"

#include "shaderthing-p/include/app.h"
#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/shareduniforms.h"
#include "shaderthing-p/include/bytedata.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"

#include <string>

namespace ShaderThing
{

void Frontend::initialize(App& app)
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
            (void*)ByteData::Font::CousineRegularData,
            ByteData::Font::CousineRegularSize, 
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );
        config.MergeMode = true;
        config.RasterizerMultiply = 1.25;
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData, 
            ByteData::Font::CousineRegularSize,
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesCyrillic()
        );
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData, 
            ByteData::Font::CousineRegularSize,
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
            (void*)ByteData::Font::FontAwesome5FreeSolid900Data, 
            ByteData::Font::FontAwesome5FreeSolid900Size, 
            iconFontSize,
            &iconConfig, 
            iconRanges
        );

        io.Fonts->Build();
        fontLoaded = true;
        font->Scale = 0.6;
        app.gui.fontScale = &font->Scale;
    }
}

//----------------------------------------------------------------------------//

void Frontend::renderAppGUI(App* app)
{
    ImGui::SetNextWindowSize(ImVec2(750,900), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin("Control panel", NULL, flags);

    renderAppMenuBarGUI(app);

    renderLayerTabBarGUI(app->layers, app->sharedUniforms);

    ImGui::Text("Here we go!");
    ImGui::End();
    ImGui::ShowDemoWindow();
}

//----------------------------------------------------------------------------//

void Frontend::renderAppMenuBarGUI(App* app)
{
    float fontSize = ImGui::GetFontSize();
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
            {
            }
            if (ImGui::MenuItem("Load", "Ctrl+O"))
            {
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
            }
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
            {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export"))
            {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::Text("Resolution");
                ImGui::SameLine();
                ImGui::PushItemWidth(8.0*fontSize);
                glm::ivec2 resolution = 
                    app->sharedUniforms.cpuBlock.iResolution;
                if 
                (
                    ImGui::InputInt2
                    (
                        "##windowResolution", 
                        glm::value_ptr(resolution)
                    )
                )
                {
                    Backend::constrainAndSetWindowResolution(resolution);
                    app->sharedUniforms.cpuBlock.iResolution = resolution;
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }
            for (auto layer : app->layers)
            {
                if (ImGui::BeginMenu(("Layer ["+layer->gui.name+"]").c_str()))
                {
                    Frontend::renderLayerSettingsGUI(layer);
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
                    app->gui.fontScale,
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
                    &app->sharedUniforms.flags.isTimeResetOnRenderingRestart
                );
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        /*
        if (ImGui::BeginMenu("Resources"))
        {
            OZ_MENU_ENTRY(resourceManager_, "Resource manager")
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
            if (ImGui::BeginMenu("System info"))
            {
                ImGui::Text("Graphics card in use:");
                ImGui::SameLine();
                ImGui::Text
                (
                    vir::GlobalPtr<vir::Renderer>::instance()->
                    deviceName().c_str()
                );
                ImGui::Text("Graphics context:    ");
                ImGui::SameLine();
                ImGui::Text
                (
                    vir::GlobalPtr<vir::Window>::instance()->
                    context()->name().c_str()
                );
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        */
        ImGui::EndMenuBar();
    }
}

//----------------------------------------------------------------------------//

void Frontend::renderLayerSettingsGUI(Layer* layer)
{
    // Render layer name input text (if changed, it is updated later in 
    // renderLayerTabBarGUI to avoid issues with random layer tab re-ordering)
    {
        ImGui::Text("Name ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", layer->id);
        if (ImGui::InputText(label.get(), &layer->gui.newName))
            layer->gui.rename = true;
    }
}

//----------------------------------------------------------------------------//

void Frontend::renderLayerTabBarGUI
(
    std::vector<Layer*>& layers,
    SharedUniforms& sharedUniforms
)
{
    // Logic for compilation button and, possibly, summary of compilation errors  
    static ImVec4 redColor = {1,0,0,1};
    static bool atLeastOneCompilationError(false);
    static bool atLeasOneUncompiledChange(false);
    if (atLeasOneUncompiledChange || atLeastOneCompilationError)
    {
        float time = vir::GlobalPtr<vir::Time>::instance()->outerTime();
        static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
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
            markAllShadersForCompilation();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("Compile all shaders");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
        ImGui::Text("Ctrl+B");
        ImGui::PopStyleColor();
    }
    bool errorColorPushed = false;
    if (atLeastOneCompilationError)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, redColor);
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    atLeastOneCompilationError = false;
    atLeasOneUncompiledChange = false;
    auto& sharedCompilationErrors
    (
        Layer::GUI::sharedSourceEditor.GetErrorMarkers()
    );
    if (sharedCompilationErrors.size() > 0)
    {
        if (!atLeastOneCompilationError)
            atLeastOneCompilationError = true;
        ImGui::Bullet();ImGui::Text("Common");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            for (auto& error : sharedCompilationErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
    for (auto* layer : layers)
    {
        auto& compilationErrors(layer->gui.sourceEditor.GetErrorMarkers());
        if (compilationErrors.size() > 0 || layer->gui.errorsInSourceHeader)
        {
            if (!atLeastOneCompilationError)
                atLeastOneCompilationError = true;
            ImGui::Bullet();ImGui::Text(layer->gui.name.c_str());
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                if (layer->gui.errorsInSourceHeader)
                {
                    std::string errorText =  
"Header: invalid uniform declaration(s), edit uniform name(s)";
                    ImGui::Text(errorText.c_str());
                }
                for (auto& error : compilationErrors)
                {
                    // First is line no., second is actual error text
                    std::string errorText = 
                        "Line "+std::to_string(error.first)+": "+error.second;
                    ImGui::Text(errorText.c_str());
                }
                ImGui::EndTooltip();
            }
        }
        if (layer->hasUncompiledChanges() && !atLeasOneUncompiledChange)
            atLeasOneUncompiledChange = true;
    }
    if (atLeastOneCompilationError)
        ImGui::Separator();
    if (errorColorPushed)
        ImGui::PopStyleColor();

    static bool reorderable(true);
    /*if (ImGui::Button("Compile layers"))
    {
        for (auto layer : layers)
        {
            Backend::compileLayerShader(layer, sharedUniforms);
        }
    }*/
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
        if (!reorderable)
            reorderable = true;
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            Backend::createLayerIn(layers, sharedUniforms);
        auto tabBar = ImGui::GetCurrentTabBar();
        std::pair<unsigned int, unsigned int> swap {0,0};
        for (int i = 0; i < layers.size(); i++)
        {
            bool open = true;
            auto l = layers[i];
            if(ImGui::BeginTabItem(l->gui.name.c_str(), &open))
            {
                Frontend::renderLayerTabGUI(l);
                ImGui::EndTabItem();
            }
            if (!open) // I.e., if 'x' is pressed to delete the tab
            {
                Backend::deleteLayerFrom(l, layers);
                --i;
                continue;
            }
            auto tab = ImGui::TabBarFindTabByID
            (
                tabBar, 
                ImGui::GetID(l->gui.name.c_str())
            );
            if // the name has changed, make the tab bar non-reorderable
            (  // and update the name
                tab == nullptr || 
                (l->gui.rename && !ImGui::GetIO().WantTextInput)
            )
            {
                reorderable = false;
                l->gui.name = l->gui.newName;
                l->gui.rename = false;
                continue;
            }
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
            auto tmp = layers[swap.first];
            layers[swap.first] = layers[swap.second];
            layers[swap.second] = tmp;
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

void Frontend::renderLayerTabGUI(Layer* layer)
{
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (ImGui::BeginTabItem("Fragment source"))
        {
            layer->gui.sourceEditor.Render("##sourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            layer->gui.sharedSourceEditor.Render("##sharedSourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

}