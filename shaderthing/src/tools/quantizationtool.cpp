#include "tools/quantizationtool.h"
#include "shaderthingapp.h"
#include "layers/layer.h"

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
layers_(app.layersRef()),
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

void QuantizationTool::loadState(std::string& source, uint32_t& index)
{
    reset();
    
    std::string lineSource;
    while(true)
    {
        char& c = source[index];
        if (c == '\n')
            break;
        lineSource += c;
        index++;
    }
    index++;
    auto targetLayerName = new char[lineSource.size()];
    int isActive, isTargetLayerValid, autoUpdatePalette, isPaletteValid, 
        isAlphaCutoff;
    sscanf
    (
        lineSource.c_str(),
        "%d %d %s %d %f %f %d %d %d %d %d %s",
        &isActive,
        &isTargetLayerValid,
        &(targetLayerName[0]), 
        &ditheringLevel_,
        &ditheringThreshold_,
        &clusteringFidelity_,
        &isAlphaCutoff,
        &alphaCutoffThreshold_,
        &autoUpdatePalette,
        &paletteSize_,
        &isPaletteValid,
        nullptr
    );
    isActive_ = bool(isActive);
    autoUpdatePalette_ = bool(autoUpdatePalette);
    isAlphaCutoff_ = bool(isAlphaCutoff);
    if (isTargetLayerValid)
    {
        for (auto layer : layers_)
        {
            if (std::strcmp(layer->nameRef().c_str(), targetLayerName)==0)
            {
                targetLayer_ = layer;
                break;
            }
        }
    }
    delete[] targetLayerName;
    if (isPaletteValid)
    {
        uIntPalette_ = new unsigned char[3*paletteSize_];
        floatPalette_ = new float[3*paletteSize_];
        for (int i=0; i<3*paletteSize_; i++)
        {
            int j=3*paletteSize_-i-1;
            uIntPalette_[j] = (unsigned char)lineSource[lineSource.size()-i-1];
            floatPalette_[j] = uIntPalette_[j]/255.0;
        }
        paletteModified_ = !autoUpdatePalette_;
    }
    firstQuantization_ = !isPaletteValid;
}

//----------------------------------------------------------------------------//

void QuantizationTool::saveState(std::ofstream& file)
{
    std::string sPalette("NULL");
    bool validPalette(uIntPalette_ != nullptr);
    std::string targetBufferName("NULL");
    bool validTargetLayer(targetLayer_ != nullptr);
    if (validTargetLayer)
        targetBufferName = targetLayer_->name();
    auto data = new char[100+(int)3*paletteSize_+(int)targetBufferName.size()];
    sprintf
    (
        data, 
        "%d %d %s %d %.9f %.9f %d %d %d %d %d", 
        (int)isActive_,
        (int)validTargetLayer,
        targetBufferName.c_str(),
        ditheringLevel_,
        ditheringThreshold_,
        clusteringFidelity_,
        (int)isAlphaCutoff_,
        alphaCutoffThreshold_,
        (int)autoUpdatePalette_,
        paletteSize_,
        (int)validPalette
    );
    file << data; //std::endl;
    if (validPalette)
    {
        file << " ";
        for (int i=0; i<3*paletteSize_; i++)
            file << uIntPalette_[i];
    }
    file << std::endl;
    delete[] data;
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

//----------------------------------------------------------------------------//

void QuantizationTool::update()
{
}

}