#include "postprocess/blurpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"
#include "data/data.h"

typedef vir::Blurrer::Settings Settings;

namespace ShaderThing
{

void BlurPostProcess::renderGui()
{
    if (!isGuiOpen_)
        return;
    
    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Blur", &isGuiOpen_, windowFlags);
        static bool setIcon (false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "Blur", 
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
            (isActive_ ? "Blur on" : "Blur off"), 
            ImVec2(-1, 0.0f)
        )
    )
        isActive_ = !isActive_;

    ImGui::Text("Radius   ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    int radius = settings_.xRadius;
    if
    (
        ImGui::DragInt
        (
            "##blurRadiusSlider",
            &radius,
            .1f,
            1   
        )
    )
    {
        settings_.xRadius = std::max(radius, 1);
        settings_.yRadius = settings_.xRadius;
    }
    ImGui::PopItemWidth();

    ImGui::Text("Sub-steps");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    ImGui::SliderInt
    (
        "##blurSubStepsDrag",
        (int*)&settings_.subSteps, 
        1,
        5,
        "%.d",
        ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::PopItemWidth();

    if (!isGuiInMenu_)
        ImGui::End();
}

}