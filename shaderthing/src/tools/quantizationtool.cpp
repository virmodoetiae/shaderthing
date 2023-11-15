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

#include "tools/quantizationtool.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Static members ------------------------------------------------------------//

std::unordered_map<int, std::string> 
    QuantizationTool::ditheringLevelToName = {
        {2, "Ordered (Bayer) 4x4"},
        {1, "Ordered (Bayer) 2x2"},
        {0, "None"}
    };

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

QuantizationTool::QuantizationTool(ShaderThingApp& app) :
app_(app),
isGuiOpen_(false),
isGuiInMenu_(true),
isActive_(false),
firstQuantization_(true),
autoUpdatePalette_(true),
isAlphaCutoff_(false),
paletteSize_(4),
ditheringLevel_(0),
clusteringFidelity_(0.9f),
ditheringThreshold_(0.25f),
alphaCutoffThreshold_(128),
quantizer_(nullptr),
uIntPalette_(nullptr),
floatPalette_(nullptr),
paletteModified_(false),
targetLayer_(nullptr)
{}

QuantizationTool::~QuantizationTool()
{
    if (quantizer_ != nullptr)
        delete quantizer_;
    quantizer_ = nullptr;
    if (uIntPalette_ != nullptr)
        delete[] uIntPalette_;
    uIntPalette_ = nullptr;
    if (floatPalette_ != nullptr)
        delete[] floatPalette_;
    floatPalette_ = nullptr;
}

//----------------------------------------------------------------------------//

bool QuantizationTool::canRunOnDeviceInUse() 
{
    if (quantizer_ == nullptr)  // Ehhh don't hate on me, If we could only have
                                // virtual static functions in C++ this would
                                // not have to look like this
        quantizer_ = vir::KMeansQuantizer::create();
    return quantizer_->canRunOnDeviceInUse();
}

const std::string& QuantizationTool::errorMessage()
{
    if (quantizer_ == nullptr)  // Ehhh don't hate on me, If we could only have
                                // virtual static functions in C++ this would
                                // not have to look like this
        quantizer_ = vir::KMeansQuantizer::create();
    return quantizer_->errorMessage();
}

//----------------------------------------------------------------------------//

void QuantizationTool::reset()
{
    isGuiOpen_ = false;
    isActive_ = false;
    firstQuantization_ = true;
    autoUpdatePalette_ = true;
    targetLayer_ = nullptr;
    if (quantizer_ != nullptr)
        delete quantizer_;
    quantizer_ = nullptr;
    if (uIntPalette_ != nullptr)
        delete[] uIntPalette_;
    uIntPalette_ = nullptr;
    if (floatPalette_ != nullptr)
        delete[] floatPalette_;
    floatPalette_ = nullptr;
    paletteModified_ = false;
    paletteSize_ = 4;
    ditheringLevel_ = 0;
    clusteringFidelity_ = 0.9f;
}

//----------------------------------------------------------------------------//

void QuantizationTool::removeLayerAsTarget(Layer* layer)
{
    if (layer == targetLayer_)
        targetLayer_ = nullptr;
}

//----------------------------------------------------------------------------//

void QuantizationTool::loadState(const ObjectIO& reader)
{
    reset();
    if (!reader.hasMember("quantizer"))
        return;
    auto quantizerData = reader.readObject("quantizer");
    isActive_ = quantizerData.read<bool>("active");
    std::string targetName = quantizerData.read("targetLayer", false);
    for (auto l : app_.layersRef())
    {
        if (l->name() != targetName)
            continue;
        targetLayer_ = l;
        break;
    }
    ditheringLevel_ = quantizerData.read<int>("ditheringLevel");
    ditheringThreshold_ = quantizerData.read<float>("ditheringThreshold");
    clusteringFidelity_ = quantizerData.read<float>("clusteringFidelity");
    isAlphaCutoff_ = quantizerData.read<bool>("transparencyCutoff");
    alphaCutoffThreshold_ = quantizerData.read<int>(
        "transparencyCutoffThreshold");
    autoUpdatePalette_ = quantizerData.read<bool>("dynamicPalette");
    firstQuantization_ = true;
    if (!quantizerData.hasMember("paletteData"))
        return;
    unsigned int paletteSizeX3;
    uIntPalette_ = (unsigned char*)quantizerData.read("paletteData", true, 
        &paletteSizeX3);
    paletteSize_ = paletteSizeX3/3;
    floatPalette_ = new float[paletteSizeX3];
    for (int i=0; i<paletteSizeX3; i++)
        floatPalette_[i] = (float)uIntPalette_[i]/255.0;
    paletteModified_ = !autoUpdatePalette_;
    firstQuantization_ = false;
}

//----------------------------------------------------------------------------//

void QuantizationTool::saveState(ObjectIO& writer)
{
    if (targetLayer_ == nullptr)
        return;
    writer.writeObjectStart("quantizer");
    writer.write("active", isActive_);
    writer.write("targetLayer", targetLayer_->name().c_str());
    writer.write("ditheringLevel", (int)ditheringLevel_);
    writer.write("ditheringThreshold", ditheringThreshold_);
    writer.write("clusteringFidelity", clusteringFidelity_);
    writer.write("transparencyCutoff", isAlphaCutoff_);
    writer.write("transparencyCutoffThreshold", alphaCutoffThreshold_);
    writer.write("dynamicPalette", autoUpdatePalette_);
    if (uIntPalette_ != nullptr)
        writer.write
        (
            "paletteData", 
            (const char*)uIntPalette_, 
            3*paletteSize_, 
            true
        );
    writer.writeObjectEnd(); // End of quantizer
}

//----------------------------------------------------------------------------//

void QuantizationTool::quantize(Layer* layer)
{
    if (targetLayer_ == nullptr)
        return;
    if (layer->id() != targetLayer_->id() || !isActive_)
        return;
    if (quantizer_ == nullptr)
        quantizer_ = vir::KMeansQuantizer::create();
    if (layer->rendersTo() != Layer::RendersTo::Window)
    {
        static int paletteSize0(-1);
        float clusteringTolerance
        (
            1.0f-clusteringFidelity_*clusteringFidelity_
        );
        vir::KMeansQuantizer::Options options = {};
        options.paletteData = paletteModified_ ? uIntPalette_ : nullptr;
        options.reseedPalette = firstQuantization_;
        options.recalculatePalette = autoUpdatePalette_ || firstQuantization_;
        options.ditherMode = 
            (vir::KMeansQuantizer::Options::DitherMode)ditheringLevel_;
        options.ditherThreshold = ditheringThreshold_;
        options.relTol = clusteringTolerance;
        options.alphaCutoff = isAlphaCutoff_ ? alphaCutoffThreshold_ : -1;
        options.regenerateMipmap = true;
        options.fastKMeans = true;
        quantizer_->quantize
        (
            targetLayer_->writeOnlyFramebuffer(),
            paletteSize_,
            options
        );
        bool paletteSizeModified(paletteSize0!=paletteSize_);
        if (paletteSizeModified)
        {
            if (uIntPalette_ != nullptr)
                delete[] uIntPalette_;
            uIntPalette_ = nullptr;
            if (floatPalette_ != nullptr)
                delete[] floatPalette_;
            floatPalette_ = nullptr;
            paletteSize0 = paletteSize_;
            floatPalette_ = new float[3*paletteSize_];
            // Re-alloc of uIntPalette happens in quanizer_->getPalette
            // uIntPalette_ = new unsigned char[3*paletteSize_]; 
        }
        quantizer_->getPalette(uIntPalette_, paletteSizeModified);
        for (int i = 0; i < 3*paletteSize_; i++)
            floatPalette_[i] = uIntPalette_[i]/255.0f;
    }
    if (firstQuantization_)
        firstQuantization_ = false;
    if (paletteModified_)
        paletteModified_ = false;
}

}