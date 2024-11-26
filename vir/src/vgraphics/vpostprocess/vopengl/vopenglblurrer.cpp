#include "vpch.h"
#include "vgraphics/vpostprocess/vopengl/vopenglblurrer.h"
#include "vgraphics/vcore/vopengl/vopenglmisc.h"

namespace vir
{

typedef TextureBuffer::InternalFormat InternalFormat;
typedef TextureBuffer::FilterMode FilterMode;
typedef TextureBuffer::WrapMode WrapMode;

bool OpenGLBlurrer::computeShaderStagesCompiled_ = false;

//----------------------------------------------------------------------------//
// Unused compute shaders

OpenGLComputeShader
    OpenGLBlurrer::blurrerSF32_
(R"(#version 430 core
                                     uniform int   d;
                                     uniform int   r;
                                     uniform int   ss;
                                     uniform ivec2 sz;
layout(binding=0)                    uniform sampler2D tx;
layout(rgba32f, binding=1) writeonly uniform image2D   im;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x < sz.x && texel.y < sz.y)
    {
        vec2 uv = vec2(texel+.5f)/sz;
        vec2 ts = d == 0 ? vec2(1.0/sz.x, 0.0) : vec2(0.0, 1.0/sz.y);
        vec4 color = texture(tx, uv);
        float sum = 1.f;
        for(int i = 1; i <= r*ss; i++)
        {
            float x = float(i)/ss;
            float g = exp(-(x*x)/(.21715f*r*r)); // Value at x=r is 0.01
            sum += g;
            x*= (1.f+float(r*r)/900);
            color += .5*g*texture(tx, uv+x*ts);
            color += .5*g*texture(tx, uv-x*ts);
        }
        imageStore(im, texel, color/sum);
    }
})");

OpenGLComputeShader
    OpenGLBlurrer::blurrerUI8_
(R"(#version 430 core
                                     uniform int   d;
                                     uniform int   r;
                                     uniform int   ss;
                                     uniform ivec2 sz;
layout(binding=0)                    uniform sampler2D tx;
layout(rgba8ui, binding=0) writeonly uniform uimage2D  im;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x < sz.x && texel.y < sz.y)
    {
        vec2 uv = vec2(texel+.5f)/sz;
        vec2 ts = d == 0 ? vec2(1.0/sz.x, 0.0) : vec2(0.0, 1.0/sz.y);
        vec4 color = texture(tx, uv);
        float sum = 1.f;
        for(int i = 1; i <= r*ss; i++)
        {
            float x = float(i)/ss;
            float g = exp(-(x*x)/(.21715f*r*r)); // Value at x=r is 0.01
            sum += g;
            x*= (1.f+float(r*r)/900);
            color += .5*g*texture(tx, uv+x*ts);
            color += .5*g*texture(tx, uv-x*ts);
        }
        imageStore(im, texel, uvec4(255*color/sum));
    }
})");

//
OpenGLBlurrer::OpenGLBlurrer() :
buffer_(nullptr)
{
    CHECK_OPENGL_COMPUTE_SHADERS_AVAILABLE
    if (computeShaderStagesCompiled_)
        return;
    blurrerSF32_.compile();
    blurrerUI8_.compile();
    computeShaderStagesCompiled_ = true;
}

//
OpenGLBlurrer::~OpenGLBlurrer()
{
    if (buffer_ == nullptr)
        return;
    delete buffer_;
    buffer_ = nullptr;
}

//
void OpenGLBlurrer::blur
(
    const Framebuffer* input,
    Settings& settings
)
{
    // Size output to match input
    prepareOutput(input);

    // Size buffer
    glm::ivec2 size(input->width(), input->height());
    bool updateBuffer(false);
    if (buffer_ == nullptr)
        updateBuffer = true;
    else if 
    (
        buffer_->width() != (uint32_t)size.x || 
        buffer_->height() != (uint32_t)size.y ||
        buffer_->wrapMode(0) != input->colorBufferWrapMode(0) ||
        buffer_->wrapMode(1) != input->colorBufferWrapMode(1)
    )
    {
        updateBuffer = true;
        delete buffer_;
    }
    if (updateBuffer)
    {
        buffer_ = TextureBuffer2D::create
        (
            nullptr,
            size.x,
            size.y,
            InternalFormat::RGBA_SF_32
        );
        buffer_->setMagFilterMode(input->colorBufferMagFilterMode());
        buffer_->setMinFilterMode(input->colorBufferMinFilterMode());
        for (int i=0;i<2;i++)
            buffer_->setWrapMode(i, input->colorBufferWrapMode(i));
    }
    
    bool isSF32(input->colorBufferInternalFormat()==InternalFormat::RGBA_SF_32);

    // Bindings --------------------------------------------------------------//
    static const unsigned int inputUnit = 0;
    static const unsigned int bufferUnit = 1;
    static const unsigned int outputUnit = 2;
    glActiveTexture(GL_TEXTURE0+inputUnit);
    glBindTexture(GL_TEXTURE_2D, input->colorBufferId());
    glActiveTexture(GL_TEXTURE0+bufferUnit);
    glBindTexture(GL_TEXTURE_2D, buffer_->id());
    glActiveTexture(GL_TEXTURE0+outputUnit);
    glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
    OpenGLComputeShader* blurrer_ = isSF32 ? &blurrerSF32_ : &blurrerUI8_;

#define N_WORK_GROUPS_X(x) std::ceil(float(x)/8)
#define N_WORK_GROUPS_Y(y) std::ceil(float(y)/8)
#define N_WORK_GROUPS_Z 1

    // Horizontal blur pass --------------------------------------------------//
    glBindImageTexture
    (
        bufferUnit,
        buffer_->id(),
        0,
        GL_FALSE,
        0,
        GL_WRITE_ONLY,
        isSF32 ? GL_RGBA32F : GL_RGBA8UI
    );
    blurrer_->setUniformInt("d", 0);
    blurrer_->setUniformInt("r", settings.xRadius, false);
    blurrer_->setUniformInt("ss", settings.subSteps, false);
    blurrer_->setUniformInt2("sz", size, false);
    blurrer_->setUniformInt("tx", inputUnit, false);
    blurrer_->setUniformInt("im", bufferUnit, false);
    blurrer_->run
    (
        N_WORK_GROUPS_X(size.x),
        N_WORK_GROUPS_Y(size.y),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();

    // Vertical blur pass --------------------------------------------------//
    glBindImageTexture
    (
        outputUnit,
        output_->colorBufferId(),
        0,
        GL_FALSE,
        0,
        GL_WRITE_ONLY,
        isSF32 ? GL_RGBA32F : GL_RGBA8UI
    );
    blurrer_->setUniformInt("d", 1, false);
    blurrer_->setUniformInt("r", settings.yRadius, false);
    blurrer_->setUniformInt("ss", settings.subSteps, false);
    blurrer_->setUniformInt2("sz", size, false);
    blurrer_->setUniformInt("tx", bufferUnit, false); // <-
    blurrer_->setUniformInt("im", outputUnit, false);
    blurrer_->run
    (
        N_WORK_GROUPS_X(size.x),
        N_WORK_GROUPS_Y(size.y),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();
    glGenerateMipmap(GL_TEXTURE_2D);
};

}