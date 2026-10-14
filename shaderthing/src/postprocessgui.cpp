/*
 _____________________
|                     |  This file is part of ShaderThingOld - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthing/include/postprocess.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/objectio.h"

#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

void QuantizationPostProcess::renderGui()
{    
    float fontSize(ImGui::GetFontSize());
    float entryWidth = 12.5f*fontSize;
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
    auto paletteSize0(currentPalette_.nColors);
    if 
    (
        ImGui::SliderInt
        (
            "##paletteSizeSlider", 
            (int*)&currentPalette_.nColors, 
            2, 
            32
        )
    )
    {
        if (currentPalette_.nColors != paletteSize0)
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
    float tol = settings_.relTol; tol*=tol; tol*=tol;
    float fidelity = 1.0-tol;
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
        settings_.relTol = std::sqrt(std::sqrt(1.0f-fidelity));
    ImGui::PopItemWidth();

    if (!settings_.recalculatePalette)
        if (ImGui::Button("Refresh palette", ImVec2(-1, 0.0f)))
            refreshPalette_ = true;

    if (!settings_.recalculatePalette || currentPalette_.data != nullptr)
        ImGui::SeparatorText("Palette");
    if (!settings_.recalculatePalette)
    {
        static int selectedUserPaletteIndex = -1;
        static int selectedDefaultPaletteIndex = -1;
        static int deletePaletteIndex = -1;
        static float deletePaletteTimer = 0.f;
        bool deletePalette = false;
        if (ImGui::Button(ICON_FA_FOLDER_OPEN, {0,0}))
            ImGui::OpenPopup("##LoadPaletteFromLibrary");
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Load a palette from the palette library");
            ImGui::EndTooltip();
        }
        if (ImGui::BeginPopup("##LoadPaletteFromLibrary"))
        {
            unsigned int nUserPalettes = 
                QuantizationPostProcess::userPalettes_.size();
            unsigned int nDefaultPalettes = 
                QuantizationPostProcess::defaultPalettes_.size();
            ImGui::SeparatorText("User palettes");
            for (auto i=0u; i<nUserPalettes; i++)
            {
                auto& palette = QuantizationPostProcess::userPalettes_[i];
                ImGui::PushID(i);
                // Press & hold for 1 second to actually delete to avoid
                // having to manage modals and stuff from within a popup of a 
                // popup to prevent accidental deletion
                ImGui::Button(ICON_FA_TRASH);
                bool buttonHeld = ImGui::IsItemActive();
                if 
                (
                    ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                    ImGui::BeginTooltip()
                )
                {
                    ImGui::Text("Hold to delete palette");
                    ImGui::EndTooltip();
                }
                if (buttonHeld)
                {
                    deletePaletteTimer += 
                        vir::Window::instance()->time()->outerTimestep();
                    if (ImGui::BeginTooltip())
                    {
                        ImGui::ProgressBar
                        (
                            deletePaletteTimer/1.f, 
                            {10*fontSize, 0}
                        );
                        ImGui::EndTooltip();
                    }
                    if (deletePaletteTimer > 1.f)
                    {
                        deletePalette = true;
                        deletePaletteIndex = i;
                        deletePaletteTimer = 0.f;
                    }
                }
                ImGui::PopID();
                ImGui::SameLine();
                if (ImGui::Selectable(palette.name.c_str()))
                {
                    selectedUserPaletteIndex = i;
                    ImGui::CloseCurrentPopup();
                }
                for (auto j=0u; j<palette.nColors; j++)
                {
                    float color[3] = 
                    {
                        palette.data[3*j]/255.0f,
                        palette.data[3*j+1]/255.0f,
                        palette.data[3*j+2]/255.0f
                    };
                    ImGui::ColorEdit3
                    (
                        std::to_string(i).c_str(), 
                        color, 
                        ImGuiColorEditFlags_NoInputs | 
                        ImGuiColorEditFlags_NoLabel | 
                        ImGuiColorEditFlags_NoSidePreview |
                        ImGuiColorEditFlags_NoTooltip
                    );
                    if (j < palette.nColors-1)
                        ImGui::SameLine(0, 0);
                }
                if (i < nUserPalettes-1)
                    ImGui::Separator();
            }
            ImGui::SeparatorText("Built-in palettes");
            for (auto i=0u; i<nDefaultPalettes; i++)
            {
                auto& palette = QuantizationPostProcess::defaultPalettes_[i];
                if (ImGui::Selectable(palette.name.c_str()))
                {
                    selectedDefaultPaletteIndex = i;
                    ImGui::CloseCurrentPopup();
                }
                for (auto j=0u; j<palette.nColors; j++)
                {
                    float color[3] = 
                    {
                        palette.data[3*j]/255.0f,
                        palette.data[3*j+1]/255.0f,
                        palette.data[3*j+2]/255.0f
                    };
                    ImGui::ColorEdit3
                    (
                        std::to_string(i).c_str(), 
                        color, 
                        ImGuiColorEditFlags_NoInputs |
                        ImGuiColorEditFlags_NoLabel |
                        ImGuiColorEditFlags_NoSidePreview |
                        ImGuiColorEditFlags_NoTooltip
                    );
                    if (j < palette.nColors-1)
                        ImGui::SameLine(0, 0);
                }
                if (i < nDefaultPalettes-1)
                    ImGui::Separator();
            }
            ImGui::EndPopup();
        }
        if (selectedUserPaletteIndex != -1)
        {
            currentPalette_ = 
                QuantizationPostProcess::userPalettes_
                [
                    selectedUserPaletteIndex
                ];
            paletteModified_ = true;
            selectedUserPaletteIndex = -1;
        }
        if (selectedDefaultPaletteIndex != -1)
        {
            currentPalette_ = 
                QuantizationPostProcess::defaultPalettes_
                [
                    selectedDefaultPaletteIndex
                ];
            paletteModified_ = true;
            selectedDefaultPaletteIndex = -1;
        }
        else if (deletePalette)
        {
            QuantizationPostProcess::userPalettes_.erase
            (
                QuantizationPostProcess::userPalettes_.begin()+
                deletePaletteIndex
            );
            deletePaletteIndex = -1;
        }

        ImGui::SameLine();
        
        static int foundPaletteIndex = -1;
        if (ImGui::Button(ICON_FA_SAVE, {0,0}))
        {
            for 
            (
                auto i=0u; 
                i<(unsigned int)QuantizationPostProcess::userPalettes_.size();
                i++
            )
            {
                if 
                (
                    QuantizationPostProcess::userPalettes_[i].name ==
                    currentPalette_.name
                )
                {
                    foundPaletteIndex = i;
                    break;
                }
            }
            if (foundPaletteIndex != -1)
                ImGui::OpenPopup("Palette overwrite confirmation");
            else
                QuantizationPostProcess::userPalettes_.emplace_back
                (
                    currentPalette_
                );
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Save palette to the palette library");
            ImGui::EndTooltip();
        }
        if (ImGui::BeginPopupModal("Palette overwrite confirmation"))
        {
            ImGui::Text
            (
                "Do you want to overwrite palette '%s'?", 
                QuantizationPostProcess::userPalettes_[foundPaletteIndex].name
                .c_str()
            );
            if (ImGui::Button("Confirm"))
            {
                QuantizationPostProcess::userPalettes_[foundPaletteIndex] = 
                    currentPalette_;
                foundPaletteIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                foundPaletteIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputText
        (
            "##currentQuantizerPaletteName",
            &currentPalette_.name
        );
        ImGui::PopItemWidth();
    }
    if (currentPalette_.data == nullptr)
        return;
    ImGui::Dummy({-1, 0});
    for (auto i=0u; i<currentPalette_.nColors; i++)
    {
        std::string id = "##paletteColorNo"+std::to_string(i);
        float color[3] = 
        {
            currentPalette_.data[3*i]/255.0f,
            currentPalette_.data[3*i+1]/255.0f,
            currentPalette_.data[3*i+2]/255.0f
        };
        float availableWidth = ImGui::GetContentRegionAvail().x;
        if 
        (
            ImGui::ColorEdit3
            (
                id.c_str(), 
                color, 
                ImGuiColorEditFlags_NoInputs | 
                ImGuiColorEditFlags_NoLabel | 
                ImGuiColorEditFlags_Uint8
            ) && !settings_.recalculatePalette
        )
        {
            paletteModified_ = true;
            currentPalette_.data[3*i] = 
                (unsigned char)(color[0]*255.0+.5f);
            currentPalette_.data[3*i+1] = 
                (unsigned char)(color[1]*255.0+.5f);
            currentPalette_.data[3*i+2] = 
                (unsigned char)(color[2]*255.0+.5f);
        }
        if 
        (
            //(i+1)%(int)(std::ceil(std::sqrt(currentPalette_.nColors))) != 0
            availableWidth >= 2*fontSize
        )
            ImGui::SameLine(0, 0);
    }
    /*
    static Palette quantizedCumulatedPalette = {};
    ImGui::NewLine();
    if (ImGui::Button("Quantize cumulated palette"))
    {
        settings_.recalculatePalette = false;   
        if (quantizedCumulatedPalette.data == nullptr)
        {
            quantizedCumulatedPalette.data = new unsigned char[1];
        }
        else
        {
            delete quantizedCumulatedPalette.data;
            quantizedCumulatedPalette.data = new unsigned char[1];
        }
        ((nativeType*)native_)->getPalette
        (
            quantizedCumulatedPalette.data, 
            true,
            true
        );
        quantizedCumulatedPalette.nColors = currentPalette_.nColors;
    }
    if (quantizedCumulatedPalette.data != nullptr)
    {
        for (auto i=0u; i<quantizedCumulatedPalette.nColors; i++)
        {
            std::string id = "##quantizedPaletteColorNo"+std::to_string(i);
            float color[3] = 
            {
                quantizedCumulatedPalette.data[3*i]/255.0f,
                quantizedCumulatedPalette.data[3*i+1]/255.0f,
                quantizedCumulatedPalette.data[3*i+2]/255.0f
            };
            float availableWidth = ImGui::GetContentRegionAvail().x;
            ImGui::ColorEdit3
            (
                id.c_str(), 
                color, 
                ImGuiColorEditFlags_NoInputs | 
                ImGuiColorEditFlags_NoLabel | 
                ImGuiColorEditFlags_Uint8
            );
            if 
            (
                //(i+1)%(int)(std::ceil(std::sqrt(currentPalette_.nColors))) != 0
                availableWidth >= 2*fontSize
            )
                ImGui::SameLine(0, 0);
        }

        ImGui::Image
        (
            (void*)(uintptr_t)
            (
                ((nativeType*)native_)->getCumulatedPaletteImageId()
            ),
            {256, 256},
            {0,1},
            {1,0}
        );
    }
    */
}

//----------------------------------------------------------------------------//

void BloomPostProcess::renderGui()
{    
    float fontSize(ImGui::GetFontSize());
    float entryWidth = 12.0f*fontSize;

    if 
    (
        ImGui::Button
        (
            (isActive_ ? "Bloom on" : "Bloom off"), 
            ImVec2(-1, 0.0f)
        )
    )
        isActive_ = !isActive_;

    ImGui::Text("Radius     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    ImGui::SliderInt
    (
        "##bloomMipDepthSlider",
        (int*)&settings_.mipDepth,
        1,
        vir::Bloomer::maxMipLevel(*inputFramebuffer_),
        "%d", 
        ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::PopItemWidth();
    
    ImGui::Text("Intensity  ");
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

    ImGui::Text("Threshold  ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomThresholdDrag", 
            &settings_.threshold, 
            .0025f, 
            0.f
        )
    )
        settings_.threshold = std::max(0.f, settings_.threshold);
    ImGui::PopItemWidth();

    ImGui::Text("Knee       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomKneeDrag", 
            &settings_.knee, 
            .0025f, 
            0.f
        )
    )
        settings_.knee = std::max(0.f, settings_.knee);
    ImGui::PopItemWidth();

    ImGui::Text("Haze       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::DragFloat
        (
            "##bloomHazeDrag",
            &settings_.haze, 
            .01f, 
            0.f
        )
    )
        settings_.haze = std::max(0.f, settings_.haze);
    ImGui::PopItemWidth();

    ImGui::Text("Tone map   ");
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


    if (settings_.toneMap == Settings::ToneMap::Reinhard)
    {
        ImGui::Text("White pt.  ");
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
    }
}

//----------------------------------------------------------------------------//

void BlurPostProcess::renderGui()
{
    float fontSize(ImGui::GetFontSize());
    float entryWidth = 12.0f*fontSize;

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
}

}