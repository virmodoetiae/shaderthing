#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include "unordered_map"
#include "string"

#include "vir/include/vgraphics/vpostprocess/vpostprocess.h"

namespace vir
{
    class Framebuffer;
}

typedef vir::PostProcess::Type Type;

namespace ShaderThing
{

class ShaderThingApp;
class Layer;
class ObjectIO;

class PostProcess
{
protected:

    // Ref to top-level app
    ShaderThingApp& app_;

    // Native post processing resource (of the vir:: library)
    vir::PostProcess* nativePostProcess_;

    // Name of this post-processing effect. For the time being, bind it to type
    std::string name_;
    
    // True if this post-processing effect is running
    bool isActive_ = false;
    
    // Source layer to be post-processed
    Layer* inputLayer_ = nullptr;
    
    // True if this post-processing effect's GUI is currently open
    bool isGuiOpen_ = false;

    // True if this post-processing effect's GUI automatically opens when
    // hovering over its entry in the main application menu bar
    bool isGuiInMenu_ = true;

    // Delete default construct
    PostProcess() = delete;

    // Private construct, only meant to be constructed via PostProcess::create
    PostProcess
    (
        ShaderThingApp& app, 
        Layer* inputLayer, 
        vir::PostProcess* nativePostProcess
    );

public:
    
    // Create a post-processing effect from scratch
    static PostProcess* create
    (
        ShaderThingApp& app, 
        Layer* inputLayer, 
        Type type
    );

    // Create a post-processing effect from serialized data
    static PostProcess* create
    (
        ShaderThingApp& app, 
        Layer* inputLayer, 
        ObjectIO& reader
    );
    
    //
    virtual ~PostProcess();

    // Reset and/or de-allocate all members to default values
    virtual void reset(){};

    // Run the post-processing effect
    virtual void run() = 0;

    // Render the GUI for controlling this post-processing effect
    virtual void renderGui() = 0;

    // Serialize all object members to the provided writer object, which is
    // to be written to disk. An ObjectIO object is fundamentally a JSON file
    // in a C++ context
    virtual void saveState(ObjectIO& writer) = 0;

    // Assign the address of the post-processing output framebuffer to the 
    // input layer writeonly framebuffer. In this way, the post-processed
    // changes are instantly visible to all resources using the input layer
    // as an input of their own. This modification is overwritten anyways
    // at the start of the input layer's rendering step, during front and
    // back-buffer swapping
    void replaceInputLayerWriteOnlyFramebuffer();

    // Mostly here for legacy reasons, will probably get scrapped
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}

    // Native accessors

    //
    Type type() const {return nativePostProcess_->type();}
    
    // Return access to output framebuffer with the applied post-processing 
    // effect
    vir::Framebuffer* outputFramebuffer(){return nativePostProcess_->output();}
    
    //
    bool canRunOnDeviceInUse() const 
    {
        return nativePostProcess_->canRunOnDeviceInUse();
    }
    
    //
    std::string errorMessage() const 
    {
        return nativePostProcess_->errorMessage();
    }

    // Other accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    std::string name() const {return name_;}
    bool isActive() const {return isActive_;}
    Layer* inputLayer() const {return inputLayer_;}

    // Setters
    void setActive(bool flag){isActive_ = flag;}
};

}

#endif