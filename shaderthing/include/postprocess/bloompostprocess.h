#ifndef ST_BLOOM_POST_PROCESS_H
#define ST_BLOOM_POST_PROCESS_H

#include "postprocess/postprocess.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

class BloomPostProcess : public PostProcess
{
protected:

    // Bloomer settings
    vir::Bloomer::Settings settings_;

    //
    void resetSettings();

public:
    
    //
    BloomPostProcess(ShaderThingApp& app, Layer* inputLayer);

    //
    BloomPostProcess
    (
        ShaderThingApp& app, 
        Layer* inputLayer,
        ObjectIO& reader
    );

    //
    virtual ~BloomPostProcess();

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