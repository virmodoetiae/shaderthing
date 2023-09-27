#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "misc/misc.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

void LayerManager::renderGui()
{
    // Render layers tab bar
    static bool layersToBeRenamed(false);
    ImGuiTabBarFlags tab_bar_flags = 
        (layersToBeRenamed) ? 
        ImGuiTabBarFlags_AutoSelectNewTabs : 
        ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable;
    if (layersToBeRenamed)
        layersToBeRenamed = false;
    

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
            Misc::isCtrlKeyPressed(ImGuiKey_B)
        )
            markAllShadersForCompilation();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("Compile all shaders");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
        ImGui::Text("Ctrl+B");
        ImGui::PopStyleColor();
    }
    static bool errorColorPushed = false;
    if (atLeastOneCompilationError)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, redColor);
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    atLeastOneCompilationError = false;
    atLeasOneUncompiledChange = false;
    for (auto* layer : layers_)
    {
        auto& compilationErrors(layer->compilationErrors());
        if (compilationErrors.size() > 0)
        {
            if (!atLeastOneCompilationError)
                atLeastOneCompilationError = true;
            ImGui::Bullet();ImGui::Text(layer->name().c_str());
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
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
    if (atLeastOneCompilationError || errorColorPushed)
    {
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // Actual rendering of layer tabs
    if (ImGui::BeginTabBar("##layerTabBar", tab_bar_flags))
    {
        // Check if a new tab (i.e., layer) should be added
        if 
        (
            ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing) || 
            layers_.size() == 0
        )
            addLayer();

        // Render gui of layer in selection
        for (auto layer : layers_)
        {
            layer->renderGui();
            layersToBeRenamed = layersToBeRenamed || 
                layer->toBeRenamed();
        }

        // Re-order layers_ vector to match tab ordering
        ImGuiTabBar* tabBar = ImGui::GetCurrentTabBar();
        std::vector<Layer*> unorderedLayers = layers_;
        int pos = 0;
        for (ImGuiTabItem& tab : tabBar->Tabs)
        {
            tab.WantClose = false;
            std::string tabName(ImGui::TabBarGetTabName(tabBar, &tab));
            Layer* layer = nullptr;
            for (auto uLayer : unorderedLayers)
            {
                if (uLayer->name() != tabName)
                    continue;
                layer = uLayer;
                break;
            }
            if (layer != nullptr)
            {
                layers_[pos] = layer;
                layer->setDepth(float(pos)*1e-3);
                pos++;
            }
        }
        ImGui::EndTabBar();
    }
}

}