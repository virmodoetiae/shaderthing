#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include "unordered_map"
#include "string"

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
        Bloom = 0,
        Quantization = 1
    };

    static std::unordered_map<Type, std::string> typeToName;

protected:

    // Ref to top-level app
    ShaderThingApp& app_;
    
    // Type of this post-processing effect
    Type type_;

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
    PostProcess(ShaderThingApp& app, Layer* inputLayer, Type type);

public:
    
    // Create a post-processing
    static PostProcess* create
    (
        ShaderThingApp& app, 
        Layer* inputLayer, 
        Type type
    );
    
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
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    Type type() const {return type_;}
    std::string name() const {return name_;}
    bool isActive() const {return isActive_;}
    Layer* inputLayer() const {return inputLayer_;}

    // Setters
    void setActive(bool flag){isActive_ = flag;}
};

}

#endif