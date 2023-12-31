#ifndef ST_BLUR_POST_PROCESS_H
#define ST_BLUR_POST_PROCESS_H

#include "postprocess/postprocess.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

class BlurPostProcess : public PostProcess
{
protected:

    // Blur settings
    vir::Blurrer::Settings settings_;

    // True if both the x and y blur radii are to be kept equal
    bool circularKernel_;

    //
    void resetSettings();

public:
    
    //
    BlurPostProcess(ShaderThingApp& app, Layer* inputLayer);

    //
    BlurPostProcess
    (
        ShaderThingApp& app, 
        Layer* inputLayer,
        ObjectIO& reader
    );

    //
    virtual ~BlurPostProcess();

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