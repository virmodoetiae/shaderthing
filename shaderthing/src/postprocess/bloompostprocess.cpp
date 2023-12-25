#include "postprocess/bloompostprocess.h"
#include "shaderthingapp.h"
#include "layers/layer.h"
#include "objectio/objectio.h"

typedef vir::Bloomer nativeType;

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
PostProcess(app, inputLayer, vir::Quantizer::create()),
settings_({})
{
    reset();
    isActive_ = reader.read<bool>("active");
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
    ((nativeType*)nativePostProcess_)->bloom
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
    writer.writeObjectEnd(); // End of bloomer
}

}