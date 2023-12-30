#include "postprocess/bloompostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

typedef vir::Bloomer NativeType;
typedef vir::Bloomer::Settings::ToneMap ToneMap;

namespace ShaderThing
{

BloomPostProcess::BloomPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer
) :
PostProcess(app, inputLayer, vir::Bloomer::create()),
settings_({})
{
    reset();
}

BloomPostProcess::BloomPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer,
    ObjectIO& reader
) :
PostProcess(app, inputLayer, vir::Bloomer::create()),
settings_({})
{
    reset();
    isActive_ = reader.read<bool>("active");
    settings_.mipDepth = reader.read<unsigned int>("mipDepth");
    settings_.intensity = reader.read<float>("intensity");
    settings_.threshold = reader.read<float>("threshold");
    settings_.knee = reader.read<float>("knee");
    settings_.haze = reader.read<float>("haze");
    settings_.toneMap = (ToneMap)reader.read<int>("toneMap");
    settings_.radmanExposure = reader.read<float>("radmanExposure");
    settings_.reinhardWhitePoint = reader.read<float>("reinhardWhitePoint");
}

BloomPostProcess::~BloomPostProcess()
{
}

void BloomPostProcess::resetSettings()
{
    settings_ = vir::Bloomer::Settings{};
}

void BloomPostProcess::reset()
{
    isGuiOpen_ = false;
    isActive_ = false;
    resetSettings();
}

void BloomPostProcess::run()
{
    if
    (
        !isActive_ || 
        inputLayer_ == nullptr ||
        !canRunOnDeviceInUse() || 
        inputLayer_->rendersTo() == Layer::RendersTo::Window
    )
        return;
    ((NativeType*)nativePostProcess_)->bloom
    (
        inputLayer_->writeOnlyFramebuffer(),
        settings_
    );

    // Essential step
    replaceInputLayerWriteOnlyFramebuffer();
}

// Serialize all object members to the provided writer object, which is
// to be written to disk. An ObjectIO object is fundamentally a JSON file
// in a C++ context
void BloomPostProcess::saveState(ObjectIO& writer)
{
    writer.writeObjectStart(nativePostProcess_->typeName().c_str());
    writer.write("active", isActive_);
    writer.write("mipDepth", settings_.mipDepth);
    writer.write("intensity", settings_.intensity);
    writer.write("threshold", settings_.threshold);
    writer.write("knee", settings_.knee);
    writer.write("haze", settings_.haze);
    writer.write("toneMap", (int)settings_.toneMap);
    writer.write("radmanExposure", settings_.radmanExposure);
    writer.write("reinhardWhitePoint", settings_.reinhardWhitePoint);
    writer.writeObjectEnd(); // End of bloomer
}

}