#ifndef V_POST_PROCESS_H
#define V_POST_PROCESS_H

#include <string>
#include <unordered_map>

namespace vir
{

class TextureBuffer2D;
class Framebuffer;

class PostProcess
{
public:

    enum class Type
    {
        Undefined = -1,
        Quantization = 0,
        Bloom = 1,
        Blur = 2
    };

    static std::unordered_map<Type, std::string> typeToName;

protected:

    //
    const Type type_ = Type::Undefined;

    // Output framebuffer
    Framebuffer* output_;

    // Depending on the post-process implementation, it might not be able to
    // run under certain systems. For example, the current implementation of
    // the OpenGLQuantizer can only run if the min available OpenGL 
    // version in use by the host system is OpenGL 4.3, because the current
    // OpenGL implentation of the quantizer runs entirely off compute shaders,
    // which are unavailable prior to OGL 4.3
    bool canRunOnDeviceInUse_;
    std::string errorMessage_;

    // Protected as this base class is not supposed to be instantiable
    // on its own
    PostProcess(Type type);

    // Delete copy-construction & copy-assignment ops
    PostProcess(const PostProcess&) = delete;
    PostProcess& operator= (const PostProcess&) = delete;

    // Copy settings of the input Framebuffer/TextureBuffer2D to the output 
    // Framebuffer. If necessary (e.g., change in size or internal data format),
    // delete and recreate the output
    void prepareOutput(const Framebuffer* input);
    void prepareOutput(const TextureBuffer2D* input);

public:

    // Destructor
    virtual ~PostProcess();

    //
    Type type() const {return type_;}

    //
    std::string typeName() const {return typeToName.at(type_);}

    // Output framebuffer for this post-processing effect
    Framebuffer* output() {return output_;}

    // True if this post-processing effect can run on this device. This might be
    // false for those post-processing effects which run e.g., on compute shaders
    // when using OpenGL as a rendering API, as compute shaders are not supported
    // below a certain OpenGL 
    bool canRunOnDeviceInUse() const {return canRunOnDeviceInUse_;}
    
    // If this post-processing effect cannot run on this device, this is the
    // related error message
    const std::string& errorMessage() const {return errorMessage_;}
};

}

#endif