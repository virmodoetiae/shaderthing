#ifndef QUANTIZATION_POST_PROCESS_H
#define QUANTIZATION_POST_PROCESS_H

#include "postprocess/postprocess.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

class QuantizationPostProcess : public PostProcess
{
protected:

    // Native KMeans quantizer
    vir::KMeansQuantizer* quantizer_;

    // Quantizer settings
    vir::KMeansQuantizer::Settings settings_;

    //
    unsigned int paletteSize_;

    // Palette data of size 3*paletteSize_, with each value in the [0,255] range
    unsigned char* uIntPalette_;

    // Same as uIntPalette_, but with each value in the [0.,1.] range
    float* floatPalette_;

    // True if the palette has been manually modified via the embedded color
    // picker in the quantization tool (can be accessed by clicking any color
    // in the displayed palette)
    bool paletteModified_;

    //
    void resetSettings();

public:
    
    //
    QuantizationPostProcess(ShaderThingApp& app, Layer* inputLayer);

    //
    virtual ~QuantizationPostProcess();

    //
    void reset() override;

    // Run the post-processing effect
    void run() override;

    // Render the GUI for controlling this post-processing effect
    void renderGui() override;

    // Return access to output framebuffer with the applied post-processing 
    // effect
    vir::Framebuffer* outputFramebuffer() override;

    // Re-initialize all object members from the data stored in the provided
    // reader object. An ObjectIO object is fundamentally a JSON file in a C++
    // context
    void loadState(const ObjectIO& reader) override;

    // Serialize all object members to the provided writer object, which is
    // to be written to disk. An ObjectIO object is fundamentally a JSON file
    // in a C++ context
    void saveState(ObjectIO& writer) override;

};

}

#endif