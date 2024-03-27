/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
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
native_(native),
inputLayer_(inputLayer),
inputFramebuffer_(&inputLayer->rendering_.framebuffer)
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
    Type type;
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
        default:
            return nullptr;
    }
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
            {155, 188, 15,
            139, 172, 15,
            48, 98, 48,
            15, 56, 15}, 
            "Game Boy"
        ),
        Palette
        (
            {0, 0, 0,
            27, 29, 30,
            39, 40, 32,
            62, 61, 50,
            117, 113, 94,
            248, 248, 242,
            249, 38, 114,
            253, 151, 31,
            230, 218, 116,
            102, 217, 239,
            166, 226, 46,
            174, 129, 255},
            "Monokai"
        )
    };

QuantizationPostProcess::Palette::Palette(const Palette& palette)
{
    nColors = palette.nColors;
    data = new unsigned char[3*nColors];
    for (int i=0; i<3*nColors; i++)
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
    for (int i=0; i<3*nColors; i++)
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
    for (int i=0; i<3*nColors; i++)
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
    DELETE_IF_NOT_NULLPTR(data)
}

//----------------------------------------------------------------------------//

bool QuantizationPostProcess::Palette::operator==(const Palette& rhs)
{
    if (nColors!=rhs.nColors || data==nullptr || rhs.data==nullptr)
        return false;
    for (int i=0; i<3*nColors; i++)
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

QuantizationPostProcess::~QuantizationPostProcess()
{
}

//----------------------------------------------------------------------------//

QuantizationPostProcess* QuantizationPostProcess::load
(
    const ObjectIO& io,
    Layer* inputLayer
)
{
    auto postProcess = new QuantizationPostProcess(inputLayer);
    postProcess->isActive_ = io.read<bool>("active");
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
    postProcess->paletteModified_ = !postProcess->settings_.recalculatePalette;
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
        paletteModified_ ? 
        currentPalette_.data : 
        nullptr;
    if (refreshPalette_)
        settings_.reseedPalette = true;
    if (settings_.reseedPalette)
        settings_.recalculatePalette = true;
    
    ((nativeType*)native_)->quantize
    (
        *inputFramebuffer_,
        currentPalette_.nColors,
        settings_
    );

    if (refreshPalette_)
    {
        refreshPalette_ = false;
        settings_.recalculatePalette = false;
    }

    if (paletteSizeModified_)
        currentPalette_.clear();
        // Re-alloc of uCharData happens in getPalette
    ((nativeType*)native_)->getPalette
    (
        currentPalette_.data, 
        paletteSizeModified_
    );
    if (settings_.reseedPalette)
        settings_.reseedPalette = false;
    if (paletteModified_)
        paletteModified_ = false;
    if (paletteSizeModified_)
        paletteSizeModified_ = false;

    // Essential step
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
            ImGui::OpenPopup("Load palette from library");
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Load a palette from the palette library");
            ImGui::EndTooltip();
        }
        if (ImGui::BeginPopup("Load palette from library"))
        {
            auto nUserPalettes = QuantizationPostProcess::userPalettes_.size();
            auto nDefaultPalettes = 
                QuantizationPostProcess::defaultPalettes_.size();
            ImGui::SeparatorText("User palettes");
            for (int i=0; i<nUserPalettes; i++)
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
                for (int j=0; j<palette.nColors; j++)
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
            for (int i=0; i<nDefaultPalettes; i++)
            {
                auto& palette = QuantizationPostProcess::defaultPalettes_[i];
                if (ImGui::Selectable(palette.name.c_str()))
                {
                    selectedDefaultPaletteIndex = i;
                    ImGui::CloseCurrentPopup();
                }
                for (int j=0; j<palette.nColors; j++)
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
            for (int i=0; i<QuantizationPostProcess::userPalettes_.size(); i++)
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
    for (int i=0; i<currentPalette_.nColors; i++)
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
    postProcess->isActive_ = io.read<bool>("active");
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

    // Essential step
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
            .0025f, 
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
            .0025f, 
            0.f
        )
    )
        settings_.knee = std::max(0.f, settings_.knee);
    ImGui::PopItemWidth();

    ImGui::Text("Haze     ");
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


    if (settings_.toneMap == Settings::ToneMap::Reinhard)
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
    postProcess->isActive_ = io.read<bool>("active");
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

    // Essential step
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