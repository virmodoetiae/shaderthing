#ifndef V_OPENGL_BLOOMER_H
#define V_OPENGL_BLOOMER_H

#include "vgraphics/vpostprocess/vbloomer.h"
#include "vgraphics/vcore/vopengl/vopenglcomputeshader.h"

namespace vir
{

class TextureBuffer2D;

class OpenGLBloomer : public Bloomer
{
protected:

    static bool computeShaderStagesCompiled_;
    static OpenGLComputeShader brightnessMask_;
    static OpenGLComputeShader downsampler_;
    static OpenGLComputeShader upsampler_;
    static OpenGLComputeShader adder_;
    static OpenGLComputeShader copyToOutput_;

    //
    TextureBuffer2D* bloom_;

    // Delete copy-construction & copy-assignment ops
    OpenGLBloomer(const OpenGLBloomer&) = delete;
    OpenGLBloomer& operator= (const OpenGLBloomer&) = delete;

public:

    //
    OpenGLBloomer();
    
    //
    virtual ~OpenGLBloomer();

    //
    void bloom
    (
        const Framebuffer* input,
        Settings settings
    ) override;

};

}

#endif