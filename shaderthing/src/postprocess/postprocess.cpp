#include "postprocess/postprocess.h"
#include "postprocess/quantizationpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

namespace ShaderThing
{

std::unordered_map<PostProcess::Type, std::string> PostProcess::typeToName = 
{
    {PostProcess::Type::Bloom, "Bloom"},
    {PostProcess::Type::Quantization, "Quantization"}
};

PostProcess::PostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer,
    Type type
):
app_(app),
inputLayer_(inputLayer),
type_(type),
name_(typeToName.at(type))
{}

PostProcess* PostProcess::create
(
    ShaderThingApp& app,
    Layer* inputLayer,
    Type type
)
{
    switch(type)
    {
        case Type::Quantization :
            return new QuantizationPostProcess(app, inputLayer);
        case Type::Bloom :
            return nullptr;
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
    for (auto kv : PostProcess::typeToName)
    {
        if (kv.second == name)
            type = kv.first;
    }
    switch(type)
    {
        case Type::Quantization :
            return new QuantizationPostProcess(app, inputLayer, reader);
        case Type::Bloom :
            return nullptr;
        default:
            return nullptr;
    }
}

void PostProcess::replaceInputLayerWriteOnlyFramebuffer()
{
    inputLayer_->writeOnlyFramebuffer_ = outputFramebuffer();
}

}