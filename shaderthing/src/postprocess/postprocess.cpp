#include "postprocess/postprocess.h"
#include "postprocess/quantizationpostprocess.h"
#include "postprocess/bloompostprocess.h"
#include "postprocess/blurpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

namespace ShaderThing
{

PostProcess::PostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer,
    vir::PostProcess* nativePostProcess
):
app_(app),
nativePostProcess_(nativePostProcess),
inputLayer_(inputLayer),
name_(nativePostProcess->typeName())
{}

PostProcess::~PostProcess()
{
    if (nativePostProcess_ == nullptr)
        return;
    delete nativePostProcess_;
    nativePostProcess_ = nullptr;
}

PostProcess* PostProcess::create
(
    ShaderThingApp& app,
    Layer* inputLayer,
    Type type
)
{
    switch(type)
    {
        case vir::PostProcess::Type::Quantization :
            return new QuantizationPostProcess(app, inputLayer);
        case vir::PostProcess::Type::Bloom :
            return new BloomPostProcess(app, inputLayer);
        case vir::PostProcess::Type::Blur :
            return new BlurPostProcess(app, inputLayer);
        default:
            return nullptr;
    }
}

// Create a post-processing effect from serialized data
PostProcess* PostProcess::create
(
    ShaderThingApp& app,
    Layer* inputLayer,
    ObjectIO& reader
)
{
    std::string name = reader.name();
    Type type;
    for (auto kv : vir::PostProcess::typeToName)
    {
        if (kv.second == name)
            type = kv.first;
    }
    switch(type)
    {
        case vir::PostProcess::Type::Quantization :
            return new QuantizationPostProcess(app, inputLayer, reader);
        case vir::PostProcess::Type::Bloom :
            return new BloomPostProcess(app, inputLayer, reader);
        case vir::PostProcess::Type::Blur :
            return new BlurPostProcess(app, inputLayer, reader);
        default:
            return nullptr;
    }
}

void PostProcess::replaceInputLayerWriteOnlyFramebuffer()
{
    inputLayer_->writeOnlyFramebuffer_ = outputFramebuffer();
}

}