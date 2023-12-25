#include "postprocess/bloompostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"
#include "data/data.h"

typedef vir::Bloomer::Settings Settings;

namespace ShaderThing
{

void BloomPostProcess::renderGui()
{
    if (!isGuiOpen_)
        return;
    
    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Bloom", &isGuiOpen_, windowFlags);
        static bool setIcon (false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "Bloom", 
                IconData::sTIconData, 
                IconData::sTIconSize,
                false
            );
        }
    }
    
    float fontSize(ImGui::GetFontSize());
    float entryWidth = isGuiInMenu_ ? 12.0f*fontSize : -1;

    if 
    (
        ImGui::Button
        (
            (isActive_ ? "Bloom on" : "Bloom off"), 
            ImVec2(entryWidth, 0.0f)
        )
    )
        isActive_ = !isActive_;

    ImGui::Text("Threshold");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if (
    ImGui::DragFloat
    (
        "##bloomThresholdSlider", 
        &settings_.threshold, 
        .0025f, 
        0.f
    ))
        settings_.threshold = std::max(0.f, settings_.threshold);
    ImGui::PopItemWidth();

    ImGui::Text("Knee     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if (
    ImGui::DragFloat
    (
        "##bloomKneeSlider", 
        &settings_.knee, 
        .0025f, 
        0.f
    ))
        settings_.knee = std::max(0.f, settings_.knee);
    ImGui::PopItemWidth();

    if (!isGuiInMenu_)
        ImGui::End();
}

}