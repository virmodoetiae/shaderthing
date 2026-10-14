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

PostProcess::PostProcess
(
    WPtr<Layer> inputLayer,
    vir::UniquePtr<vir::PostProcess>&& native
) :
inputLayer_(inputLayer),
inputFramebuffer_(&inputLayer_->renderState_.resourceFramebuffer),
native_(std::move(native))
{}

//----------------------------------------------------------------------------//

PostProcess::~PostProcess()
{
    if (isActive_ && inputLayer_.valid())
        *inputFramebuffer_ = 
            inputLayer_->renderState_.framebufferA.get();
}

//----------------------------------------------------------------------------//

vir::UniquePtr<PostProcess> PostProcess::create
(
    WPtr<Layer> inputLayer,
    Type type
)
{
    switch(type)
    {
        case Type::Quantization :
            return vir::makeUnique<QuantizationPostProcess>(inputLayer);
        case Type::Bloom :
            return vir::makeUnique<BloomPostProcess>(inputLayer);
        case Type::Blur :
            return vir::makeUnique<BlurPostProcess>(inputLayer);
        default :
            return vir::nullUniquePtr<PostProcess>();
    }
    return vir::nullUniquePtr<PostProcess>();
}

//----------------------------------------------------------------------------//

UPtr<PostProcess> PostProcess::loadFrom
(
    const ObjectIO& io,
    WPtr<Layer> inputLayer
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
            return QuantizationPostProcess::loadFrom(io, inputLayer);
        case Type::Bloom :
            return BloomPostProcess::loadFrom(io, inputLayer);
        case Type::Blur :
            return BlurPostProcess::loadFrom(io, inputLayer);
        case Type::Undefined:
            return vir::nullUniquePtr<PostProcess>();
    }
    return vir::nullUniquePtr<PostProcess>();
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

bool PostProcess::shouldRun() const
{
    if                                                                    
    (                                                                     
        !isActive_ ||                                                     
        inputLayer_ == nullptr ||                                         
        !canRunOnDeviceInUse() ||                                         
        inputLayer_->renderState_.target == Layer::RenderState::Target::Window
    )                                                                     
        return false;
    return true;
}

//----------------------------------------------------------------------------//

void PostProcess::overwriteInputLayerFramebuffer()
{
    *inputFramebuffer_ = outputFramebuffer();
}


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

QuantizationPostProcess::QuantizationPostProcess(WPtr<Layer> inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Quantizer::create()
)
{
    currentPalette_.nColors = 4;
}

//----------------------------------------------------------------------------//

UPtr<QuantizationPostProcess> QuantizationPostProcess::loadFrom
(
    const ObjectIO& io,
    WPtr<Layer> layer
)
{
    auto postProcess = vir::makeUnique<QuantizationPostProcess>(layer);
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

void QuantizationPostProcess::saveTo(ObjectIO& io) const
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
    if (!shouldRun())
        return;
    
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
    ((nativeType*)native_.get())->quantize
    (
        *inputFramebuffer_,
        currentPalette_.nColors,
        settings_
    );

    if (paletteSizeModified_)
        currentPalette_.clear();
    if (refreshPalette_ || settings_.recalculatePalette || paletteSizeModified_)
        ((nativeType*)native_.get())->getPalette
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
// BloomPostProcess ----------------------------------------------------------//
//----------------------------------------------------------------------------//

BloomPostProcess::BloomPostProcess(WPtr<Layer> inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Bloomer::create()
){}

//----------------------------------------------------------------------------//

UPtr<BloomPostProcess> BloomPostProcess::loadFrom
(
    const ObjectIO& io,
    WPtr<Layer> layer
)
{
    auto postProcess = vir::makeUnique<BloomPostProcess>(layer);
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

void BloomPostProcess::saveTo(ObjectIO& io) const
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
    if (!shouldRun())
        return;

    ((nativeType*)native_.get())->bloom
    (
        *inputFramebuffer_,
        settings_
    );

    overwriteInputLayerFramebuffer();
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
// BlurPostProcess -----------------------------------------------------------//
//----------------------------------------------------------------------------//

BlurPostProcess::BlurPostProcess(WPtr<Layer> inputLayer) : 
PostProcess
(
    inputLayer,
    vir::Blurrer::create()
){}

UPtr<BlurPostProcess> BlurPostProcess::loadFrom
(
    const ObjectIO& io, 
    WPtr<Layer> layer
)
{
    auto postProcess = vir::makeUnique<BlurPostProcess>(layer);
    postProcess->isActive_ 
        = io.read<bool>("active") && postProcess->canRunOnDeviceInUse();
    postProcess->settings_.xRadius = io.read<unsigned int>("xRadius");
    postProcess->settings_.yRadius = io.read<unsigned int>("yRadius");
    postProcess->settings_.subSteps = io.read<unsigned int>("subSteps");
    postProcess->isKernelCircular_ = io.read<bool>("circularKernel");
    return postProcess;
}

//----------------------------------------------------------------------------//

void BlurPostProcess::saveTo(ObjectIO& io) const
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
    if (!shouldRun())
        return;

    ((nativeType*)native_.get())->blur
    (
        *inputFramebuffer_,
        settings_
    );

    overwriteInputLayerFramebuffer();
}

}