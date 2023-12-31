#include "postprocess/blurpostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

typedef vir::Blurrer NativeType;

namespace ShaderThing
{

BlurPostProcess::BlurPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer
) :
PostProcess(app, inputLayer, NativeType::create()),
settings_({}),
circularKernel_(true)
{
    reset();
}

BlurPostProcess::BlurPostProcess
(
    ShaderThingApp& app,
    Layer* inputLayer,
    ObjectIO& reader
) :
PostProcess(app, inputLayer, NativeType::create()),
settings_({}),
circularKernel_(true)
{
    reset();
    isActive_ = reader.read<bool>("active");
    settings_.xRadius = reader.read<unsigned int>("xRadius");
    settings_.yRadius = reader.read<unsigned int>("yRadius");
    settings_.subSteps = reader.read<unsigned int>("subSteps");
    circularKernel_ = reader.read<bool>("circularKernel");
}

BlurPostProcess::~BlurPostProcess()
{
}

void BlurPostProcess::resetSettings()
{
    settings_ = NativeType::Settings{};
    circularKernel_ = true;
}

void BlurPostProcess::reset()
{
    isGuiOpen_ = false;
    isActive_ = false;
    resetSettings();
}

void BlurPostProcess::run()
{
    if
    (
        !isActive_ || 
        inputLayer_ == nullptr ||
        !canRunOnDeviceInUse() || 
        inputLayer_->rendersTo() == Layer::RendersTo::Window
    )
        return;
    ((NativeType*)nativePostProcess_)->blur
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
void BlurPostProcess::saveState(ObjectIO& writer)
{
    writer.writeObjectStart(nativePostProcess_->typeName().c_str());
    writer.write("active", isActive_);
    writer.write("xRadius", settings_.xRadius);
    writer.write("yRadius", settings_.yRadius);
    writer.write("subSteps", settings_.subSteps);
    writer.write("circularKernel", circularKernel_);
    writer.writeObjectEnd();
}

}