#ifndef V_OPENGL_BLURRER_H
#define V_OPENGL_BLURRER_H

#include "vgraphics/vpostprocess/vblurrer.h"
#include "vgraphics/vcore/vopengl/vopenglcomputeshader.h"

namespace vir
{

class TextureBuffer2D;

class OpenGLBlurrer : public Blurrer
{
protected:

    // True if all compute shader stages have been compiled on first class
    // instantiation
    static bool computeShaderStagesCompiled_;

    //
    static OpenGLComputeShader blurrerSF32_;
    static OpenGLComputeShader blurrerUI8_;

    // An intermediate processing texture
    TextureBuffer2D* buffer_;

    // Delete copy-construction & copy-assignment ops
    OpenGLBlurrer(const OpenGLBlurrer&) = delete;
    OpenGLBlurrer& operator= (const OpenGLBlurrer&) = delete;

public:

    // Constructor
    OpenGLBlurrer();
    
    // Destructor
    virtual ~OpenGLBlurrer();

    // Bloom
    void blur
    (
        const Framebuffer* input,
        Settings& settings
    ) override;

};

}

#endif