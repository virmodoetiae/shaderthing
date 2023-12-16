#include "postprocess/postprocess.h"
#include "postprocess/quantizationpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

namespace ShaderThing
{

std::unordered_map<PostProcess::Type, std::string> PostProcess::typeToName = 
{
    {PostProcess::Type::Quantization, "Quantization"},
    {PostProcess::Type::Bloom, "Bloom"}
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

}