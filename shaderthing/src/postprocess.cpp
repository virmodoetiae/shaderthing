/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
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

PostProcess::PostProcess
(
    Layer* inputLayer,
    vir::PostProcess* native
):
inputLayer_(inputLayer),
inputFramebuffer_(&inputLayer->rendering_.resourceFramebuffer),
native_(native)
{}

//----------------------------------------------------------------------------//

PostProcess::~PostProcess()
{
    DELETE_IF_NOT_NULLPTR(native_)
    if (isActive_)
        *inputFramebuffer_ = 
            inputLayer_->rendering_.framebufferA;
}

//----------------------------------------------------------------------------//

PostProcess* PostProcess::create
(
    Layer* inputLayer,
    Type type
)
{
    switch(type)
    {
        case Type::Quantization :
            return new QuantizationPostProcess(inputLayer);
        case Type::Bloom :
            return new BloomPostProcess(inputLayer);
        case Type::Blur :
            return new BlurPostProcess(inputLayer);
        default :
            return nullptr;
    }
    return nullptr;
}

//----------------------------------------------------------------------------//

PostProcess* PostProcess::load
(
    const ObjectIO& io,
    Layer* inputLayer
)
{
    std::string typeName = io.name();
    Type type = Type::Undefined;
    for (auto kv : vir::PostProcess::typeToName)
    {
        if (kv.second == typeName)
            type = kv.first;
    }
    switch(type)
    {
        case Type::Quantization :
            return QuantizationPostProcess::load(io, inputLayer);
        case Type::Bloom :
            return BloomPostProcess::load(io, inputLayer);
        case Type::Blur :
            return BlurPostProcess::load(io, inputLayer);
        case Type::Undefined:
            return nullptr;
    }
    return nullptr;
}

//----------------------------------------------------------------------------//

void PostProcess::loadStaticData(const ObjectIO& io)
{
    if (!io.hasMember("sharedPostProcessData"))
        return;
    auto ioPostProcess = io.readObject("sharedPostProcessData");

    // Currently only QuantizationPostProcess has any data to load/save
    QuantizationPostProcess::loadUserPalettes(ioPostProcess);
}

//----------------------------------------------------------------------------//

void PostProcess::saveStaticData(ObjectIO& io)
{
    io.writeObjectStart("sharedPostProcessData");
    // Currently only QuantizationPostProcess has any data to load/save
    QuantizationPostProcess::saveUserPalettes(io);
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void PostProcess::overwriteInputLayerFramebuffer()
{
    *inputFramebuffer_ = outputFramebuffer();
}

#define CHECK_SHOULD_RUN                                                    \
    if                                                                      \
    (                                                                       \
        !isActive_ ||                                                       \
        inputLayer_ == nullptr ||                                           \
        !canRunOnDeviceInUse() ||                                           \
        inputLayer_->renderingTarget() == Layer::Rendering::Target::Window  \
    )                                                                       \
        return;

//----------------------------------------------------------------------------//
// QuantizationPostProcess ---------------------------------------------------//
//----------------------------------------------------------------------------//

std::vector<QuantizationPostProcess::Palette> 
    QuantizationPostProcess::userPalettes_ = {};
std::vector<QuantizationPostProcess::Palette> 
    QuantizationPostProcess::defaultPalettes_ = 
    {
        /*Palette
        (
            {},
            ""
        ),*/
        Palette
        (
            {33,30,32,
            85,85,104,
            160,160,139,
            233,239,236},
            "2-Bit Demichrome"
        ),
        Palette
        (
            {4,12,6,
            17,35,24,
            30,58,41,
            48,93,66,
            77,128,97,
            137,162,87,
            190,220,127,
            238,255,204},
            "Ammo"
        ),
        Palette
        (
            {252,176,140,
            239,157,127,
            214,147,138,
            180,141,146,
            165,151,161,
            143,160,191,
            154,171,201,
            165,183,212},
            "Cl8uds"
        ),
        Palette
        (
            {0, 0, 0,
            255, 255, 255,
            136, 0, 0,
            170, 255, 238,
            204, 68, 204,
            0, 204, 85,
            0, 0, 170,
            238, 238, 119,
            221, 136, 85,
            102, 68, 0,
            255, 119, 119,
            51, 51, 51,
            119, 119, 119,
            170, 255, 102,
            0, 136, 255,
            187, 187, 187},
            "Commodore 64"
        ),
        Palette
        (
            {42,23,59,
            63,44,95,
            68,63,123,
            76,92,135,
            105,128,158,
            149,197,172},
            "Cryptic Ocean"
        ),
        Palette
        (
            {22,4,2,
            76,48,18,
            88,89,49,
            177,149,76,
            239,217,139},
            "Detective Bass"
        ),
        Palette
        (
            {201,204,161,
            202,160,90,
            174,106,71,
            139,64,73,
            84,51,68,
            81,82,98,
            99,120,125,
            142,160,145},
            "Dreamscape"
        ),
        Palette
        (
            {155, 188, 15,
            139, 172, 15,
            48, 98, 48,
            15, 56, 15}, 
            "Game Boy"
        ),
        Palette
        (
            {8,24,32,
            52,104,86,
            136,192,112,
            224,248,208},
            "Game Boy (BGB)"
        ),
        Palette
        (
            {255,130,116,
            213,60,106,
            124,24,60,
            70,14,43,
            49,5,30,
            31,5,16,
            19,2,8},
            "Midnight Ablaze"
        ),
        Palette
        (
            {59,41,51,
            86,67,92,
            75,106,99,
            128,150,101,
            199,168,160,
            183,123,121,
            116,104,137,
            159,72,78},
            "Nicely Dead"
        ),
        Palette
        (
            {8,20,30,
            15,42,63,
            32,57,79,
            246,214,189,
            195,163,138,
            153,117,119,
            129,98,113,
            78,73,95},
            "Nyx8"
        ),
        Palette
        (
            {251,245,239,
            242,211,171,
            198,159,165,
            139,109,156,
            73,77,126,
            39,39,68},
            "Oil"
        ),
        Palette
        (
            {31,36,75,
            101,64,83,
            168,96,93,
            209,166,126,
            246,231,156,
            182,207,142,
            96,174,123,
            60,107,100},
            "Paper"
        ),
        Palette
        (
            {24,16,16,
            132,115,156,
            247,181,140,
            255,239,255},
            "Pokémon (SGB)"
        ),
        Palette
        (
            {209,191,176,
            122,156,150,
            72,107,127,
            68,71,96,
            57,43,53,
            66,57,82,
            113,63,82,
            187,71,79},
            "Retrotronic DX"
        ),
        Palette
        (
            {246,205,38,
            172,107,38,
            86,50,38,
            51,28,23,
            187,127,87,
            114,89,86,
            57,57,57,
            32,32,32},
            "Rust Gold"
        ),
        Palette
        (
            {13,43,69,
            32,60,86,
            84,78,104,
            141,105,122,
            208,129,89,
            255,170,94,
            255,212,163,
            255,236,214},
            "SLSO 8"
        ),
        Palette
        (
            {55,54,78,
            53,93,105,
            106,174,157,
            185,212,180,
            244,233,212,
            208,186,169,
            158,142,145,
            91,74,104},
            "Seafoam"
        ),
        Palette
        (
            {251,187,173,
            238,134,149,
            74,122,150,
            51,63,88,
            41,40,49},
            "Twilight"
        ),
        Palette
        (
            {0, 0, 0,
            130, 0, 0,
            0, 130, 0,
            130, 130, 0,
            0, 0, 130,
            130, 0, 130,
            0, 130, 130,
            195, 195, 195,
            130, 130, 130,
            255, 0, 0,
            0, 255, 0,
            255, 255, 0,
            0, 0, 255,
            255, 0, 255,
            0, 255, 255,
            255, 255, 255,},
            "Windows 3"
        ),
    };

QuantizationPostProcess::Palette::Palette(const Palette& palette)
{
    nColors = palette.nColors;
    data = new unsigned char[3*nColors];
    for (auto i=0u; i<3u*nColors; i++)
        data[i] = palette.data[i];
    name = palette.name;
}

//----------------------------------------------------------------------------//

QuantizationPostProcess::Palette::Palette
(
    std::initializer_list<unsigned int> colors,
    const char* nameCStr
)
{
    nColors = int(colors.size()/3);
    data = new unsigned char[3*nColors];
    for (auto i=0u; i<3u*nColors; i++)
        data[i] = *(colors.begin()+i);
    name = nameCStr;
}

//----------------------------------------------------------------------------//

QuantizationPostProcess::Palette& 
QuantizationPostProcess::Palette::operator=(const Palette& rhs)
{
    nColors = rhs.nColors;
    DELETE_IF_NOT_NULLPTR(data)
    data = new unsigned char[3*nColors];
    for (auto i=0u; i<3u*nColors; i++)
        data[i] = rhs.data[i];
    name = rhs.name;
    return *this;
}

//----------------------------------------------------------------------------//

QuantizationPostProcess::Palette::~Palette()
{
    clear();
}

//----------------------------------------------------------------------------//

void QuantizationPostProcess::Palette::clear()
{
    DELETE_ARRAY_IF_NOT_NULLPTR(data)
}

//----------------------------------------------------------------------------//

bool QuantizationPostProcess::Palette::operator==(const Palette& rhs)
{
    if (nColors!=rhs.nColors || data==nullptr || rhs.data==nullptr)
        return false;
    for (auto i=0u; i<3u*nColors; i++)
    {
        if (data[i] != rhs.data[i])
            return false;
    }
    return true;
}

//----------------------------------------------------------------------------//

QuantizationPostProcess::QuantizationPostProcess(Layer* inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Quantizer::create()
)
{
    currentPalette_.nColors = 4;
}

//----------------------------------------------------------------------------//

QuantizationPostProcess* QuantizationPostProcess::load
(
    const ObjectIO& io,
    Layer* inputLayer
)
{
    auto postProcess = new QuantizationPostProcess(inputLayer);
    postProcess->isActive_ 
        = io.read<bool>("active") && postProcess->canRunOnDeviceInUse();
    postProcess->settings_.ditherMode = 
        (vir::Quantizer::Settings::DitherMode)io.read<int>("ditherMode");
    postProcess->settings_.ditherThreshold = io.read<float>("ditherThreshold");
    postProcess->settings_.relTol = io.read<float>("quantizationTolerance");
    postProcess->settings_.alphaCutoff = 
        io.read<int>("transparencyCutoffThreshold");
    postProcess->settings_.recalculatePalette = io.read<bool>("dynamicPalette");
    postProcess->settings_.reseedPalette = true;
    if (!io.hasMember("paletteData"))
        return postProcess;
    unsigned int paletteSizeX3;
    postProcess->currentPalette_.data = 
        (unsigned char*)io.read("paletteData", true, &paletteSizeX3);
    postProcess->currentPalette_.nColors = paletteSizeX3/3;
    postProcess->currentPalette_.name = 
        io.readOrDefault<std::string>("paletteName", Palette{}.name);
    postProcess->paletteModified_ = true;
    postProcess->paletteSizeModified_ = false;
    postProcess->settings_.reseedPalette = false;
    return postProcess;
}

//----------------------------------------------------------------------------//

void QuantizationPostProcess::save(ObjectIO& io) const
{
    io.writeObjectStart(native_->typeName().c_str());
    io.write("active", isActive_);
    io.write("ditherMode", (int)settings_.ditherMode);
    io.write("ditherThreshold", settings_.ditherThreshold);
    io.write("quantizationTolerance", settings_.relTol);
    io.write("transparencyCutoffThreshold", settings_.alphaCutoff);
    io.write("dynamicPalette", settings_.recalculatePalette);
    if (currentPalette_.data != nullptr)
    {
        io.write
        (
            "paletteData", 
            (const char*)currentPalette_.data, 
            3*currentPalette_.nColors, 
            true
        );
        io.write("paletteName", currentPalette_.name);
    }
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void QuantizationPostProcess::loadUserPalettes(const ObjectIO& io)
{
    userPalettes_.clear();
    if (!io.hasMember("userPalettes"))
        return;
    auto ioPalettes = io.readObject("userPalettes");
    for (auto paletteName : ioPalettes.members())
    {
        auto ioPalette = ioPalettes.readObject(paletteName);
        auto& palette = userPalettes_.emplace_back(Palette{});
        palette.data = 
            (unsigned char*)ioPalette.read
            (
                "paletteData", 
                true, 
                &palette.nColors
            );
        palette.nColors /= 3;
        palette.name = paletteName;
    }
}

//----------------------------------------------------------------------------//

void QuantizationPostProcess::saveUserPalettes(ObjectIO& io)
{
    if (userPalettes_.size() == 0)
        return;
    io.writeObjectStart("userPalettes");
    for (auto& palette : userPalettes_)
    {
        io.writeObjectStart(palette.name.c_str());
        io.write
        (
            "paletteData", 
            (const char*)palette.data, 
            3*palette.nColors, 
            true
        );
        io.writeObjectEnd();
    }
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void QuantizationPostProcess::run()
{
    CHECK_SHOULD_RUN
    
    settings_.paletteData = 
        (paletteModified_ || !settings_.recalculatePalette) ?
        currentPalette_.data : 
        nullptr;
    if (refreshPalette_)
    {
        settings_.reseedPalette = true;
        settings_.paletteData = nullptr;
    }
    if (settings_.reseedPalette)
        settings_.recalculatePalette = true;
    auto minFilterMode = (*inputFramebuffer_)->colorBufferMinFilterMode();
    if 
    (
        minFilterMode != FilterMode::Nearest &&
        minFilterMode != FilterMode::Linear
    )
        settings_.regenerateMipmap = true;
    else
        settings_.regenerateMipmap = false;
    //settings_.cumulatePalette = true;
    ((nativeType*)native_)->quantize
    (
        *inputFramebuffer_,
        currentPalette_.nColors,
        settings_
    );

    if (paletteSizeModified_)
        currentPalette_.clear();
    if (refreshPalette_ || settings_.recalculatePalette || paletteSizeModified_)
        ((nativeType*)native_)->getPalette
        (
            currentPalette_.data, 
            paletteSizeModified_
        );
    if (refreshPalette_)
    {
        refreshPalette_ = false;
        settings_.recalculatePalette = false;
    }
    settings_.reseedPalette = false;
    paletteModified_ = false;
    paletteSizeModified_ = false;

    overwriteInputLayerFramebuffer();
}

//----------------------------------------------------------------------------//

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
// BloomPostProcess ----------------------------------------------------------//
//----------------------------------------------------------------------------//

BloomPostProcess::BloomPostProcess(Layer* inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Bloomer::create()
){}

//----------------------------------------------------------------------------//

BloomPostProcess* BloomPostProcess::load
(
    const ObjectIO& io,
    Layer* inputLayer
)
{
    auto postProcess = new BloomPostProcess(inputLayer);
    postProcess->isActive_ 
        = io.read<bool>("active") && postProcess->canRunOnDeviceInUse();
    postProcess->settings_.mipDepth = io.read<unsigned int>("mipDepth");
    postProcess->settings_.intensity = io.read<float>("intensity");
    postProcess->settings_.threshold = io.read<float>("threshold");
    postProcess->settings_.knee = io.read<float>("knee");
    postProcess->settings_.haze = io.read<float>("haze");
    postProcess->settings_.toneMap = (Settings::ToneMap)io.read<int>("toneMap");
    postProcess->settings_.reinhardWhitePoint = 
        io.read<float>("reinhardWhitePoint");
    return postProcess;
}

//----------------------------------------------------------------------------//

void BloomPostProcess::save(ObjectIO& io) const
{
    io.writeObjectStart(native_->typeName().c_str());
    io.write("active", isActive_);
    io.write("mipDepth", settings_.mipDepth);
    io.write("intensity", settings_.intensity);
    io.write("threshold", settings_.threshold);
    io.write("knee", settings_.knee);
    io.write("haze", settings_.haze);
    io.write("toneMap", (int)settings_.toneMap);
    io.write("reinhardWhitePoint", settings_.reinhardWhitePoint);
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void BloomPostProcess::run()
{
    CHECK_SHOULD_RUN

    ((nativeType*)native_)->bloom
    (
        *inputFramebuffer_,
        settings_
    );

    overwriteInputLayerFramebuffer();
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
// BlurPostProcess -----------------------------------------------------------//
//----------------------------------------------------------------------------//

BlurPostProcess::BlurPostProcess(Layer* inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Blurrer::create()
){}

//----------------------------------------------------------------------------//

BlurPostProcess* BlurPostProcess::load(const ObjectIO& io, Layer* inputLayer)
{
    auto postProcess = new BlurPostProcess(inputLayer);
    postProcess->isActive_ 
        = io.read<bool>("active") && postProcess->canRunOnDeviceInUse();
    postProcess->settings_.xRadius = io.read<unsigned int>("xRadius");
    postProcess->settings_.yRadius = io.read<unsigned int>("yRadius");
    postProcess->settings_.subSteps = io.read<unsigned int>("subSteps");
    postProcess->isKernelCircular_ = io.read<bool>("circularKernel");
    return postProcess;
}

//----------------------------------------------------------------------------//

void BlurPostProcess::save(ObjectIO& io) const
{
    io.writeObjectStart(native_->typeName().c_str());
    io.write("active", isActive_);
    io.write("xRadius", settings_.xRadius);
    io.write("yRadius", settings_.yRadius);
    io.write("subSteps", settings_.subSteps);
    io.write("circularKernel", isKernelCircular_);
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void BlurPostProcess::run()
{
    CHECK_SHOULD_RUN

    ((nativeType*)native_)->blur
    (
        *inputFramebuffer_,
        settings_
    );

    overwriteInputLayerFramebuffer();
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