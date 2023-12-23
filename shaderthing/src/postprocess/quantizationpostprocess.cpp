#include "postprocess/quantizationpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

typedef vir::Quantizer nativeType;

namespace ShaderThing
{

QuantizationPostProcess::QuantizationPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer
) :
PostProcess(app, inputLayer, vir::Quantizer::create()),
settings_({}),
paletteSize_(4),
uIntPalette_(nullptr),
floatPalette_(nullptr),
paletteModified_(false),
refreshPalette_(false)
{
    reset();
}

QuantizationPostProcess::QuantizationPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer,
    ObjectIO& reader
) :
PostProcess(app, inputLayer, vir::Quantizer::create()),
settings_({}),
paletteSize_(4),
uIntPalette_(nullptr),
floatPalette_(nullptr),
paletteModified_(false),
refreshPalette_(false)
{
    reset();
    isActive_ = reader.read<bool>("active");
    settings_.ditherMode = 
        (vir::Quantizer::Settings::DitherMode)reader.read<int>("ditherMode");
    settings_.ditherThreshold = reader.read<float>("ditherThreshold");
    settings_.relTol = reader.read<float>("quantizationTolerance");
    settings_.alphaCutoff = reader.read<int>(
        "transparencyCutoffThreshold");
    settings_.recalculatePalette = reader.read<bool>("dynamicPalette");
    settings_.reseedPalette = true;
    if (!reader.hasMember("paletteData"))
        return;
    unsigned int paletteSizeX3;
    uIntPalette_ = (unsigned char*)reader.read("paletteData", true, 
        &paletteSizeX3);
    paletteSize_ = paletteSizeX3/3;
    floatPalette_ = new float[paletteSizeX3];
    for (int i=0; i<paletteSizeX3; i++)
        floatPalette_[i] = (float)uIntPalette_[i]/255.0;
    paletteModified_ = !settings_.recalculatePalette;
    settings_.reseedPalette = false;
}

QuantizationPostProcess::~QuantizationPostProcess()
{
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
    settings_.ditherMode = vir::Quantizer::Settings::DitherMode::None;
    settings_.alphaCutoff = -1;
    settings_.regenerateMipmap = true;
    settings_.fastKMeans = true;
    settings_.overwriteInput = false;
    settings_.relTol = .5f;
    settings_.ditherThreshold = .25f;
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
        !canRunOnDeviceInUse() || 
        inputLayer_->rendersTo() == Layer::RendersTo::Window
    )
        return;
    static int paletteSize0(-1);
    settings_.paletteData = paletteModified_ ? uIntPalette_ : nullptr;
    if (refreshPalette_)
        settings_.reseedPalette = true;
    if (settings_.reseedPalette)
        settings_.recalculatePalette = true;
    
    ((nativeType*)nativePostProcess_)->quantize
    (
        inputLayer_->writeOnlyFramebuffer(),
        paletteSize_,
        settings_
    );

    if (refreshPalette_)
    {
        refreshPalette_ = false;
        settings_.recalculatePalette = false;
    }

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
    ((nativeType*)nativePostProcess_)->getPalette
    (
        uIntPalette_, 
        paletteSizeModified
    );
    for (int i = 0; i < 3*paletteSize_; i++)
        floatPalette_[i] = uIntPalette_[i]/255.0f;
    if (settings_.reseedPalette)
        settings_.reseedPalette = false;
    if (paletteModified_)
        paletteModified_ = false;

    // Essential step
    replaceInputLayerWriteOnlyFramebuffer();
}

// Serialize all object members to the provided writer object, which is
// to be written to disk. An ObjectIO object is fundamentally a JSON file
// in a C++ context
void QuantizationPostProcess::saveState(ObjectIO& writer)
{
    writer.writeObjectStart(nativePostProcess_->typeName().c_str());
    writer.write("active", isActive_);
    writer.write("ditherMode", (int)settings_.ditherMode);
    writer.write("ditherThreshold", settings_.ditherThreshold);
    writer.write("quantizationTolerance", settings_.relTol);
    writer.write("transparencyCutoffThreshold", settings_.alphaCutoff);
    writer.write("dynamicPalette", settings_.recalculatePalette);
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

}