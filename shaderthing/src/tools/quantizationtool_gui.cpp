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
#include "tools/quantizationtool.h"
#include "data/data.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

void QuantizationTool::renderGui()
{
    if (!isGuiOpen_)
        return;
    
    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,350), ImGuiCond_FirstUseEver);
        //ImGui::SetNextWindowBgAlpha(0.25f);
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

    float x0 = ImGui::GetCursorPosX();
    ImGui::Text("Quantize layer      ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    std::string targetLayerName("");
    if (targetLayer_ != nullptr)
        targetLayerName = targetLayer_->nameRef();
    if 
    (
        ImGui::BeginCombo
        (
            "##targetLayerSelector", 
            targetLayerName.c_str()
        )
    )
    {
        for(auto layer : app_.layersRef())
        {
            if (layer->rendersTo() == Layer::RendersTo::Window)
                continue;
            if (ImGui::Selectable(layer->nameRef().c_str()))
            {
                targetLayer_ = layer;
                firstQuantization_ = true;
                break;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if 
    (
        ImGui::Button
        (
            (isActive_ ? "Quantizer on" : "Quantizer off"), 
            ImVec2(-1, 0.0f)
        ) && targetLayer_ != nullptr
    )
        isActive_ = !isActive_;

    if (targetLayer_ == nullptr)
    {
        if (!isGuiInMenu_)
            ImGui::End();
        return;
    }

    ImGui::Text("Transparency mode   ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::Button
        (
            isAlphaCutoff_ ? "Binary cutoff" : "Same as layer",
            ImVec2(-1,0)
        )
    )
        isAlphaCutoff_ = !isAlphaCutoff_;
    ImGui::PopItemWidth();
    if (isAlphaCutoff_)
    {
        ImGui::Text("Transparency cutoff ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        ImGui::SliderInt
        (
            "##alphaCutoffThreshold", 
            &alphaCutoffThreshold_, 
            0, 
            254
        );
        ImGui::PopItemWidth();
    }

    ImGui::Text("Palette size        ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    ImGui::SliderInt("##paletteSizeSlider", &paletteSize_, 2, 16);
    ImGui::PopItemWidth();

    ImGui::Text("Color dithering     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##ditheringSelector", 
            ditheringLevelToName[ditheringLevel_].c_str()
        )
    )
    {
        for(auto e : ditheringLevelToName)
        {
            if (ImGui::Selectable(e.second.c_str()))
            {
                ditheringLevel_ = e.first;
                break;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (ditheringLevel_ > 0)
    {
        ImGui::Text("Dithering threshold ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        ImGui::SliderFloat
        (
            "##ditheringThreshold", 
            &ditheringThreshold_, 
            0.001f, 
            1.0f
        );
        ImGui::PopItemWidth();
    }

    ImGui::Text("Dynamic palette     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    ImGui::Checkbox("##autoUpdatePalette", &autoUpdatePalette_);
    ImGui::PopItemWidth();

    ImGui::Text("Palette fidelity    ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    static float clusteringFidelity0(clusteringFidelity_);
    ImGui::SliderFloat("##clusterFidelity", &clusteringFidelity_, 0.0f, 1.0f);
    if (clusteringFidelity0 != clusteringFidelity_)
    {
        clusteringFidelity_ = std::max(clusteringFidelity_, 0.0f);
        clusteringFidelity_ = std::min(clusteringFidelity_, 1.0f);
        clusteringFidelity0 = clusteringFidelity_;
    }
    ImGui::PopItemWidth();

    if (!autoUpdatePalette_)
        if (ImGui::Button("Refresh palette", ImVec2(-1, 0.0f)))
            firstQuantization_ = true;

    if (floatPalette_ != nullptr)
    {
        ImGui::SeparatorText("Current palette");
        for (int i=0; i<paletteSize_; i++)
        {
            std::string id = "##paletteColorNo"+std::to_string(i);
            bool paletteModified(false);
            if 
            (
                ImGui::ColorEdit3
                (
                    id.c_str(), 
                    floatPalette_+3*i, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel
                ) && !autoUpdatePalette_
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
    if (!isGuiInMenu_)
        ImGui::End();
}

}