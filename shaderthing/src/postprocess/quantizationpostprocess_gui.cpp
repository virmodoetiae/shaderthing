#include "postprocess/quantizationpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"
#include "data/data.h"

typedef vir::Quantizer::Settings Settings;

namespace ShaderThing
{

void QuantizationPostProcess::renderGui()
{
    if (!isGuiOpen_)
        return;
    
    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Quantizer", &isGuiOpen_, windowFlags);
        static bool setIcon (false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "Quantizer", 
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
            (isActive_ ? "Quantizer on" : "Quantizer off"), 
            ImVec2(-1, 0.0f)
        )
    )
        isActive_ = !isActive_;

    ImGui::Text("Transparency mode   ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::Button
        (
            settings_.alphaCutoff != -1 ? "Binary cutoff" : "Same as layer",
            ImVec2(-1,0)
        )
    )
        settings_.alphaCutoff = settings_.alphaCutoff == -1 ? 0 : -1;
    ImGui::PopItemWidth();
    if (settings_.alphaCutoff != -1)
    {
        ImGui::Text("Transparency cutoff ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        ImGui::SliderInt
        (
            "##alphaCutoffThreshold", 
            &settings_.alphaCutoff, 
            0, 
            254
        );
        ImGui::PopItemWidth();
    }

    ImGui::Text("Palette size        ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    auto paletteSize0(paletteSize_);
    if (ImGui::SliderInt("##paletteSizeSlider", (int*)&paletteSize_, 2, 24))
    {
        if (paletteSize_ != paletteSize0)
            paletteSizeModified_ = true;
    }
    ImGui::PopItemWidth();

    ImGui::Text("Color dithering     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##ditheringSelector", 
            Settings::ditherModeToName[settings_.ditherMode].c_str()
        )
    )
    {
        for(auto e : Settings::ditherModeToName)
        {
            if (ImGui::Selectable(e.second.c_str()))
            {
                settings_.ditherMode = e.first;
                break;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (settings_.ditherMode != Settings::DitherMode::None)
    {
        ImGui::Text("Dithering threshold ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        float threshold = std::sqrt(settings_.ditherThreshold);
        if 
        (
            ImGui::SliderFloat
            (
                "##ditheringThreshold", 
                &threshold, 
                0.01f, 
                1.00f,
                "%.2f"
            )
        )
            settings_.ditherThreshold = threshold*threshold;
        ImGui::PopItemWidth();
    }

    ImGui::Text("Dynamic palette     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    ImGui::Checkbox("##autoUpdatePalette", &settings_.recalculatePalette);
    ImGui::PopItemWidth();

    ImGui::Text("Palette fidelity    ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    float fidelity = 1.0-settings_.relTol*settings_.relTol;
    if 
    (
        ImGui::SliderFloat
        (
            "##fidelitySliderFloat",
            &fidelity,
            0.01f,
            1.00f,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        )
    )
        settings_.relTol = std::sqrt(1.0f-fidelity);
    ImGui::PopItemWidth();

    if (!settings_.recalculatePalette)
        if (ImGui::Button("Refresh palette", ImVec2(-1, 0.0f)))
            refreshPalette_ = true;

    if (floatPalette_ != nullptr)
    {
        ImGui::SeparatorText("Current palette");
        for (int i=0; i<paletteSize_; i++)
        {
            std::string id = "##paletteColorNo"+std::to_string(i);
            if 
            (
                ImGui::ColorEdit3
                (
                    id.c_str(), 
                    floatPalette_+3*i, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel
                ) && !settings_.recalculatePalette
            )
            {
                paletteModified_ = true;
                uIntPalette_[3*i] = (255.0f*floatPalette_[3*i]+.5f);
                uIntPalette_[3*i+1] = (255.0f*floatPalette_[3*i+1]+.5f);
                uIntPalette_[3*i+2] = (255.0f*floatPalette_[3*i+2]+.5f);
            }
            if ((i+1) % (int)(std::ceil(std::sqrt(paletteSize_))) != 0)
                ImGui::SameLine();
        }
    }
    /*
    auto qOut = quantizer_->output();
    if (qOut!=nullptr)
    {
        ImVec2 hoverSize{256,256};
        float aspectRatio = qOut->width()/qOut->height();
        if (aspectRatio > 1.0)
            hoverSize.y /= aspectRatio;
        else
            hoverSize.x *= aspectRatio;
        float startx = ImGui::GetCursorPosX();
        ImVec2 uv0{0,1};
        ImVec2 uv1{1,0};
        ImGui::Image
        (
            (void*)(uintptr_t)(qOut->colorBufferId()), 
            hoverSize, 
            uv0, 
            uv1
        );
    }
    */
    if (!isGuiInMenu_)
        ImGui::End();
}

}