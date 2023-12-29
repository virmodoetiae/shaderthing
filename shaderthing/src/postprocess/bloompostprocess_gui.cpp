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
            ImVec2(-1, 0.0f)
        )
    )
        isActive_ = !isActive_;

    ImGui::Text("Radius   ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    ImGui::SliderInt
    (
        "##bloomMipDepthSlider",
        (int*)&settings_.mipDepth,
        1,
        vir::Bloomer::maxMipDepth
    );
    ImGui::PopItemWidth();

    ImGui::Text("Intensity");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomIntensityDrag",
            &settings_.intensity, 
            .01f, 
            0.f
        )
    )
        settings_.intensity = std::max(0.f, settings_.intensity);
    ImGui::PopItemWidth();

    ImGui::Text("Threshold");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomThresholdDrag", 
            &settings_.threshold, 
            .01f, 
            0.f
        )
    )
        settings_.threshold = std::max(0.f, settings_.threshold);
    ImGui::PopItemWidth();

    ImGui::Text("Knee     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomKneeDrag", 
            &settings_.knee, 
            .01f, 
            0.f
        )
    )
        settings_.knee = std::max(0.f, settings_.knee);
    ImGui::PopItemWidth();

    ImGui::Text("Dimming  ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomCoreDimmingDrag",
            &settings_.coreDimming, 
            .01f, 
            0.f
        )
    )
        settings_.coreDimming = std::max(0.f, settings_.coreDimming);
    ImGui::PopItemWidth();

    ImGui::Text("Tone map ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##bloomToneMapCombo",
            vir::Bloomer::toneMapToName.at(settings_.toneMap).c_str()
        )
    )
    {
        for (auto item : vir::Bloomer::toneMapToName)
        {
            if (ImGui::Selectable(item.second.c_str()))
                settings_.toneMap = item.first;
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    switch (settings_.toneMap)
    {
    case Settings::ToneMap::Reinhard :
    {
        ImGui::Text("Exposure ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::DragFloat
            (
                "##bloomReinhardExposureDrag", 
                &settings_.reinhardExposure, 
                .1f, 
                0.f
            )
        )
            settings_.reinhardExposure = 
                std::max(0.f, settings_.reinhardExposure);
        ImGui::PopItemWidth();
        break;
    }
    case Settings::ToneMap::ReinhardExtended :
    {
        ImGui::Text("White pt.");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::DragFloat
            (
                "##bloomReinhardWhitePointDrag", 
                &settings_.reinhardWhitePoint, 
                .01f, 
                0.f
            )
        )
            settings_.reinhardWhitePoint = 
                std::max(0.f, settings_.reinhardWhitePoint);
        ImGui::PopItemWidth();
        break;
    }
    default:
        break;
    }

    if (!isGuiInMenu_)
        ImGui::End();
}

}