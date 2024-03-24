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

PostProcess::~PostProcess()
{
    DELETE_IF_NOT_NULLPTR(native_)
    if (isActive_)
        *inputFramebuffer_ = 
            inputLayer_->rendering_.framebufferA;
}

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

QuantizationPostProcess::QuantizationPostProcess(Layer* inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Quantizer::create()
){}

QuantizationPostProcess::~QuantizationPostProcess()
{
    DELETE_IF_NOT_NULLPTR(uIntPalette_)
    DELETE_IF_NOT_NULLPTR(floatPalette_)
}

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
    postProcess->uIntPalette_ = (unsigned char*)io.read("paletteData", true, 
        &paletteSizeX3);
    postProcess->paletteSize_ = paletteSizeX3/3;
    postProcess->floatPalette_ = new float[paletteSizeX3];
    for (int i=0; i<paletteSizeX3; i++)
        postProcess->floatPalette_[i] = 
            (float)postProcess->uIntPalette_[i]/255.0;
    postProcess->paletteModified_ = !postProcess->settings_.recalculatePalette;
    postProcess->settings_.reseedPalette = false;
    return postProcess;
}

void QuantizationPostProcess::save(ObjectIO& io)
{
    io.writeObjectStart(native_->typeName().c_str());
    io.write("active", isActive_);
    io.write("ditherMode", (int)settings_.ditherMode);
    io.write("ditherThreshold", settings_.ditherThreshold);
    io.write("quantizationTolerance", settings_.relTol);
    io.write("transparencyCutoffThreshold", settings_.alphaCutoff);
    io.write("dynamicPalette", settings_.recalculatePalette);
    if (uIntPalette_ != nullptr)
        io.write
        (
            "paletteData", 
            (const char*)uIntPalette_, 
            3*paletteSize_, 
            true
        );
    io.writeObjectEnd();
}

void QuantizationPostProcess::run()
{
    CHECK_SHOULD_RUN
    
    settings_.paletteData = paletteModified_ ? uIntPalette_ : nullptr;
    if (refreshPalette_)
        settings_.reseedPalette = true;
    if (settings_.reseedPalette)
        settings_.recalculatePalette = true;
    
    ((nativeType*)native_)->quantize
    (
        *inputFramebuffer_,
        paletteSize_,
        settings_
    );

    if (refreshPalette_)
    {
        refreshPalette_ = false;
        settings_.recalculatePalette = false;
    }

    if (paletteSizeModified_)
    {
        if (uIntPalette_ != nullptr)
            delete[] uIntPalette_;
        uIntPalette_ = nullptr;
        if (floatPalette_ != nullptr)
            delete[] floatPalette_;
        floatPalette_ = nullptr;
        // Re-alloc of uIntPalette happens in quanizer_->getPalette
        // uIntPalette_ = new unsigned char[3*paletteSize_];
        floatPalette_ = new float[3*paletteSize_];
    }
    ((nativeType*)native_)->getPalette
    (
        uIntPalette_, 
        paletteSizeModified_
    );
    for (int i = 0; i < 3*paletteSize_; i++)
        floatPalette_[i] = uIntPalette_[i]/255.0f;
    if (settings_.reseedPalette)
        settings_.reseedPalette = false;
    if (paletteModified_)
        paletteModified_ = false;
    if (paletteSizeModified_)
        paletteSizeModified_ = false;

    // Essential step
    overwriteInputLayerFramebuffer();
}

void QuantizationPostProcess::renderGui()
{    
    float fontSize(ImGui::GetFontSize());
    float entryWidth = 12.0f*fontSize;

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
}

//----------------------------------------------------------------------------//
// BloomPostProcess ----------------------------------------------------------//

BloomPostProcess::BloomPostProcess(Layer* inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Bloomer::create()
){}

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

void BloomPostProcess::save(ObjectIO& io)
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

BlurPostProcess::BlurPostProcess(Layer* inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Blurrer::create()
){}

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

void BlurPostProcess::save(ObjectIO& io)
{
    io.writeObjectStart(native_->typeName().c_str());
    io.write("active", isActive_);
    io.write("xRadius", settings_.xRadius);
    io.write("yRadius", settings_.yRadius);
    io.write("subSteps", settings_.subSteps);
    io.write("circularKernel", isKernelCircular_);
    io.writeObjectEnd();
}

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