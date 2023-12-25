#include "vpch.h"
#include "vgraphics/vpostprocess/vopengl/vopenglbloomer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglcomputeshader.h"

namespace vir
{

bool OpenGLBloomer::computeShaderStagesCompiled_ = false;

OpenGLComputeShader
    OpenGLBloomer::brightnessMaskShader_(
R"(#version 430 core
uniform float threshold;
uniform float knee;
layout(rgba32f, binding=0) uniform image2D inputImage;
layout(rgba32f, binding=1) uniform image2D outputImage;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 tx = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);
    if (tx.x < size.x && tx.y < size.y)
    {
        vec4 col = imageLoad(inputImage, tx);
        float lmn = dot(col.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
        float f = 1.f;
        float tmk = threshold-knee;
        if (lmn <= tmk)
            f = 0.f;
        else if (lmn < threshold)
        {
            f = (lmn-tmk)/knee;
            f *= f*(3.f-2.f*f);
        }
        imageStore(outputImage, tx, vec4(f*col.rgb, col.a));
    }
})"
);

//
OpenGLBloomer::OpenGLBloomer() :
bloom_(nullptr)
{
    if (computeShaderStagesCompiled_)
        return;
    brightnessMaskShader_.compile();
    computeShaderStagesCompiled_ = true;
}

//
OpenGLBloomer::~OpenGLBloomer()
{
    if (bloom_ != nullptr)
        delete bloom_;
    bloom_ = nullptr;
}

//
void OpenGLBloomer::bloom
(
    const Framebuffer* input,
    Settings settings
)
{
    // Size output to match input
    prepareOutput(input);
    if (bloom_ == nullptr)
        bloom_ = TextureBuffer2D::create
        (
            nullptr,
            input->width(),
            input->height(),
            TextureBuffer::InternalFormat::RGBA_SF_32
        );

    // Compute brightness mask

    // Bind input
    unsigned int inputUnit = 0;
    glActiveTexture(GL_TEXTURE0+inputUnit);
    glBindTexture(GL_TEXTURE_2D, input->colorBufferId());
    glBindImageTexture
    (
        inputUnit, 
        input->colorBufferId(), 
        0, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        GL_RGBA32F
    );

    // Bind output
    unsigned int outputUnit = 1;
    glActiveTexture(GL_TEXTURE0+outputUnit);
    glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
    glBindImageTexture
    (
        outputUnit, 
        output_->colorBufferId(), 
        0, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        GL_RGBA32F
    );

    // Bind shader uniforms
    brightnessMaskShader_.setUniformFloat("threshold", settings.threshold);
    brightnessMaskShader_.setUniformFloat("knee", settings.knee);
    brightnessMaskShader_.setUniformInt("inputImage", inputUnit);
    brightnessMaskShader_.setUniformInt("outputImage", outputUnit);

    unsigned int xGroupSize = std::ceil(float(input->width())/8);
    unsigned int yGroupSize = std::ceil(float(input->height())/8);
    brightnessMaskShader_.run(xGroupSize, yGroupSize, 1);

    OpenGLComputeShader::waitSync();
    OpenGLComputeShader::resetSync();
}

}