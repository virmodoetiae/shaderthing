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

    // True if all compute shader stages have been compiled on first class
    // instantiation
    static bool computeShaderStagesCompiled_;

    // A downsampler which caches texture data for each work group before
    // downsampling, so to avoid performing a textureLod 13 times for every
    // single texel. However, it turned out it runs just as well as the 
    // simpler downsampler_, meaning that the 13 textureLod fetches are
    // marginal in computational expensiveness in the grand scheme of things.
    // I am keeping it for reference for future such compute shaders, but
    // otherwise not used because of its poor legibility/maintainability
    // compared to the simpler downsampler_;
    static OpenGLComputeShader cachingDownsampler_;

    // Actually used compute shaders
    static OpenGLComputeShader brightnessMask_;
    static OpenGLComputeShader downsampler_;
    static OpenGLComputeShader upsampler_;
    static OpenGLComputeShader adderSF32_;
    static OpenGLComputeShader adderUI8_;

    // Intermediate results texture used for a variety of purposes
    TextureBuffer2D* bloom_;

    // Delete copy-construction & copy-assignment ops
    OpenGLBloomer(const OpenGLBloomer&) = delete;
    OpenGLBloomer& operator= (const OpenGLBloomer&) = delete;

public:

    // Constructor
    OpenGLBloomer();
    
    // Destructor
    virtual ~OpenGLBloomer();

    // Bloom
    void bloom
    (
        const Framebuffer* input,
        Settings& settings
    ) override;

};

}

#endif