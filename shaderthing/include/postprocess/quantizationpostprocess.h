#ifndef ST_QUANTIZATION_POST_PROCESS_H
#define ST_QUANTIZATION_POST_PROCESS_H

#include "postprocess/postprocess.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

class QuantizationPostProcess : public PostProcess
{
protected:

    // Quantizer settings
    vir::Quantizer::Settings settings_;

    //
    unsigned int paletteSize_;

    // Palette data of size 3*paletteSize_, with each value in the [0,255] range
    unsigned char* uIntPalette_;

    // Same as uIntPalette_, but with each value in the [0.,1.] range
    float* floatPalette_;

    // True if the palette has been manually modified via the embedded color
    // picker in the quantization tool (can be accessed by clicking any color
    // in the displayed palette), regardless of palette size changes
    bool paletteModified_;

    // True if the palette size has been modified
    bool paletteSizeModified_;

    //
    bool refreshPalette_;

    //
    void resetSettings();

public:
    
    //
    QuantizationPostProcess(ShaderThingApp& app, Layer* inputLayer);

    //
    QuantizationPostProcess
    (
        ShaderThingApp& app, 
        Layer* inputLayer,
        ObjectIO& reader
    );

    //
    virtual ~QuantizationPostProcess();

    //
    void reset() override;

    // Run the post-processing effect
    void run() override;

    // Render the GUI for controlling this post-processing effect
    void renderGui() override;

    // Serialize all object members to the provided writer object, which is
    // to be written to disk. An ObjectIO object is fundamentally a JSON file
    // in a C++ context
    void saveState(ObjectIO& writer) override;

};

}

#endif