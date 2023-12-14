#ifndef POST_PROCESS_H
#define POST_PROCESS_H

namespace vir
{
    class Framebuffer;
}

namespace ShaderThing
{

class ShaderThingApp;
class Layer;
class ObjectIO;

class PostProcess
{
public:

    enum class Type
    {
        Undefined = 0,
        Bloom = 1,
        Quantization = 2
    };

protected:

    // Ref to top-level app
    ShaderThingApp& app_;
    
    // Type of this post-processing effect
    Type type_ = Type::Undefined;
    
    // True if this post-processing effect is running
    bool isActive_ = false;
    
    // Source layer to be post-processed
    Layer* inputLayer_ = nullptr;
    
    // True if this post-processing effect's GUI is currently open
    bool isGuiOpen_ = false;

    // True if this post-processing effect's GUI automatically opens when
    // hovering over its entry in the main application menu bar
    bool isGuiInMenu_ = true;

    PostProcess() = delete;

public:
    
    //
    PostProcess(ShaderThingApp& app, Layer* inputLayer):
        app_(app), inputLayer_(inputLayer){}
    
    //
    virtual ~PostProcess(){}

    // Reset and/or de-allocate all members to default values
    virtual void reset(){};

    // Run the post-processing effect
    virtual void run() = 0;

    // Render the GUI for controlling this post-processing effect
    virtual void renderGui() = 0;

    // Return access to output framebuffer with the applied post-processing 
    // effect
    virtual vir::Framebuffer* outputFramebuffer() = 0;

    // Re-initialize all object members from the data stored in the provided
    // reader object. An ObjectIO object is fundamentally a JSON file in a C++
    // context
    virtual void loadState(const ObjectIO& reader) = 0;

    // Serialize all object members to the provided writer object, which is
    // to be written to disk. An ObjectIO object is fundamentally a JSON file
    // in a C++ context
    virtual void saveState(ObjectIO& writer) = 0;

    //
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}

    // Other Accessors
    Type type() const {return type_;}
    bool isActive() const {return isActive_;}
    Layer* source() const {return inputLayer_;}

    // Setters
    void setActive(bool flag){isActive_ = flag;}
};

}

#endif