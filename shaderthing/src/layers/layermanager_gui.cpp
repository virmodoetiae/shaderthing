#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"

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