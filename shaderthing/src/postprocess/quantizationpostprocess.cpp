#include "postprocess/quantizationpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

namespace ShaderThing
{

QuantizationPostProcess::QuantizationPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer
) :
PostProcess(app, inputLayer, Type::Quantization),
quantizer_(vir::KMeansQuantizer::create()),
settings_({}),
paletteSize_(4),
uIntPalette_(nullptr),
floatPalette_(nullptr),
paletteModified_(false)
{
    type_ = PostProcess::Type::Quantization;
    reset();
}

QuantizationPostProcess::~QuantizationPostProcess()
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

void QuantizationPostProcess::resetSettings()
{
    settings_.reseedPalette = true;
    settings_.recalculatePalette = true;
    settings_.paletteData = nullptr;
    settings_.ditherMode = vir::KMeansQuantizer::Settings::DitherMode::None;
    settings_.alphaCutoff = -1;
    settings_.regenerateMipmap = true;
    settings_.fastKMeans = true;
    settings_.overwriteInput = false;
    settings_.relTol = .5f;
}

void QuantizationPostProcess::reset()
{
    isGuiOpen_ = false;
    isActive_ = false;
    if (uIntPalette_ != nullptr)
        delete[] uIntPalette_;
    uIntPalette_ = nullptr;
    if (floatPalette_ != nullptr)
        delete[] floatPalette_;
    floatPalette_ = nullptr;
    paletteModified_ = false;
    paletteSize_ = 4;
    resetSettings();
}

void QuantizationPostProcess::run()
{
    if
    (
        !isActive_ || 
        inputLayer_ == nullptr ||
        !quantizer_->canRunOnDeviceInUse() || 
        inputLayer_->rendersTo() == Layer::RendersTo::Window
    )
        return;
    static int paletteSize0(-1);
    settings_.paletteData = paletteModified_ ? uIntPalette_ : nullptr;
    if (settings_.reseedPalette)
        settings_.recalculatePalette = true;
    
    quantizer_->quantize
    (
        inputLayer_->writeOnlyFramebuffer(),
        paletteSize_,
        settings_
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
    if (settings_.reseedPalette)
        settings_.reseedPalette = false;
    if (paletteModified_)
        paletteModified_ = false;
}

// Return access to output framebuffer with the applied post-processing 
// effect
vir::Framebuffer* QuantizationPostProcess::outputFramebuffer()
{
    if (quantizer_!=nullptr)
        return quantizer_->output();
    return nullptr;
}

// Re-initialize all object members from the data stored in the provided
// reader object. An ObjectIO object is fundamentally a JSON file in a C++
// context
void QuantizationPostProcess::loadState(const ObjectIO& reader)
{

}

// Serialize all object members to the provided writer object, which is
// to be written to disk. An ObjectIO object is fundamentally a JSON file
// in a C++ context
void QuantizationPostProcess::saveState(ObjectIO& writer)
{

}

}