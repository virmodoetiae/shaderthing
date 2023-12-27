#include "vpch.h"
#include "vgraphics/vpostprocess/vopengl/vopenglbloomer.h"
#include "vgraphics/vcore/vopengl/vopenglmisc.h"

namespace vir
{

typedef TextureBuffer::InternalFormat InternalFormat;
typedef TextureBuffer::FilterMode FilterMode;
typedef TextureBuffer::WrapMode WrapMode;

bool OpenGLBloomer::computeShaderStagesCompiled_ = false;

OpenGLComputeShader
    OpenGLBloomer::brightnessMask_
    (
R"(#version 430 core
uniform float threshold;
uniform float knee;
layout(binding=0) uniform sampler2D inputTexture;
layout(rgba32f, binding=1) writeonly uniform image2D outputImage;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    if (texel.x < size.x && texel.y < size.y)
    {
        vec2 uv = vec2(texel)/size;
        vec4 col = textureLod(inputTexture, uv, 0);
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
        imageStore(outputImage, texel, vec4(f*col.rgb, col.a));
    }
})"
    );

OpenGLComputeShader
    OpenGLBloomer::downsampler_
    (
R"(#version 430 core
uniform int mipLevel;
layout(binding=0) uniform sampler2D inputTexture;
layout(rgba32f, binding=0) writeonly uniform image2D outputImage;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
vec4 downsample(sampler2D tx, vec2 uv, int mip)
{
    vec2 texelSize = 1.0 / textureSize(tx, mip);
    float x = texelSize.x;
    float y = texelSize.y;
    // Take 13 samples around current texel 'e':
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    vec4 a = textureLod(tx, vec2(uv.x - 2*x, uv.y + 2*y), mip);
    vec4 b = textureLod(tx, vec2(uv.x,       uv.y + 2*y), mip);
    vec4 c = textureLod(tx, vec2(uv.x + 2*x, uv.y + 2*y), mip);
    vec4 d = textureLod(tx, vec2(uv.x - 2*x, uv.y), mip);
    vec4 e = textureLod(tx, vec2(uv.x,       uv.y), mip);
    vec4 f = textureLod(tx, vec2(uv.x + 2*x, uv.y), mip);
    vec4 g = textureLod(tx, vec2(uv.x - 2*x, uv.y - 2*y), mip);
    vec4 h = textureLod(tx, vec2(uv.x,       uv.y - 2*y), mip);
    vec4 i = textureLod(tx, vec2(uv.x + 2*x, uv.y - 2*y), mip);
    vec4 j = textureLod(tx, vec2(uv.x - x, uv.y + y), mip);
    vec4 k = textureLod(tx, vec2(uv.x + x, uv.y + y), mip);
    vec4 l = textureLod(tx, vec2(uv.x - x, uv.y - y), mip);
    vec4 m = textureLod(tx, vec2(uv.x + x, uv.y - y), mip);
    // Weigh contributions and return
    vec4 color = e*0.125;
    color += (a+c+g+i)*0.03125;
    color += (b+d+f+h)*0.0625;
    color += (j+k+l+m)*0.125;
    return color;
}
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    if (texel.x < size.x && texel.y < size.y)
    {
        vec2 uv = (vec2(texel)+.5f)/size;
        vec4 col = downsample(inputTexture, uv, mipLevel);
        imageStore(outputImage, texel, col);
    }
})"
    );

OpenGLComputeShader
    OpenGLBloomer::copyToOutput_
    (
R"(#version 430 core
uniform int mipLevel;
layout(binding=0) uniform sampler2D inputTexture;
layout(rgba32f, binding=1) writeonly uniform image2D outputImage;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    if (texel.x < size.x && texel.y < size.y)
    {
        vec2 uv = vec2(texel)/size;
        vec4 col = textureLod(inputTexture, uv, mipLevel);
        imageStore(outputImage, texel, col);
    }
}
)");

//
OpenGLBloomer::OpenGLBloomer() :
bloom_(nullptr)
{
    if (computeShaderStagesCompiled_)
        return;
    brightnessMask_.compile();
    downsampler_.compile();
    copyToOutput_.compile();
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
    settings_ = settings;

    // Size output to match input
    prepareOutput(input);
    if (bloom_ == nullptr)
    {
        bloom_ = TextureBuffer2D::create
        (
            nullptr,
            input->width(),
            input->height(),
            InternalFormat::RGBA_SF_32
        );
        bloom_->setMagFilterMode(FilterMode::Linear);
        bloom_->setMinFilterMode(FilterMode::LinearMipmapNearest);
        for (int i=0;i<2;i++)
            bloom_->setWrapMode(i, WrapMode::ClampToEdge);
    }
    else if 
    (
        bloom_->width() != input->width() || 
        bloom_->height() != input->height()
    )
    {
        delete bloom_;
        bloom_ = TextureBuffer2D::create
        (
            nullptr,
            input->width(),
            input->height(),
            InternalFormat::RGBA_SF_32
        );
        bloom_->setMagFilterMode(FilterMode::Linear);
        bloom_->setMinFilterMode(FilterMode::LinearMipmapNearest);
        for (int i=0;i<2;i++)
            bloom_->setWrapMode(i, WrapMode::ClampToEdge);
    }

    // Brightness mask -------------------------------------------------------//
    unsigned int unit0 = 0;
    glActiveTexture(GL_TEXTURE0+unit0);
    glBindTexture(GL_TEXTURE_2D, input->colorBufferId());
    unsigned int unit1 = 1;
    glActiveTexture(GL_TEXTURE0+unit1);
    glBindTexture(GL_TEXTURE_2D, bloom_->id());
    glBindImageTexture
    (
        unit1, 
        bloom_->id(), 
        0,
        GL_FALSE, 
        0, 
        GL_WRITE_ONLY,
        GL_RGBA32F
    );
    brightnessMask_.setUniformFloat("threshold", settings.threshold);
    brightnessMask_.setUniformFloat("knee", settings.knee, false);
    brightnessMask_.setUniformInt("inputTexture", unit0, false);
    brightnessMask_.setUniformInt("outputImage", unit1, false);

#define N_WORK_GROUPS_X(x) std::ceil(float(x)/8)
#define N_WORK_GROUPS_Y(y) std::ceil(float(y)/8)
#define N_WORK_GROUPS_Z 1
    
    brightnessMask_.run
    (
        N_WORK_GROUPS_X(bloom_->width()),
        N_WORK_GROUPS_Y(bloom_->height()),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();

    // Downsampling ----------------------------------------------------------//
    glActiveTexture(GL_TEXTURE0+unit0);
    glBindTexture(GL_TEXTURE_2D, bloom_->id());
    int minSideResolution = 3;
    int finalMipLevel=1;
    while (true)
    {
        // Bind output target mip level image
        glBindImageTexture
        (
            unit0, 
            bloom_->id(), 
            finalMipLevel,
            GL_FALSE, 
            0, 
            GL_WRITE_ONLY,
            GL_RGBA32F
        );

        // Get target mip level texture size
        GLint width, height;
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D,
            finalMipLevel,
            GL_TEXTURE_WIDTH, 
            &width
        );
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D, 
            finalMipLevel,
            GL_TEXTURE_HEIGHT, 
            &height
        );
        //std::cout<<width<<" "<<height<<" "<<finalMipLevel<<std::endl;

        downsampler_.setUniformInt("mipLevel", finalMipLevel-1);
        downsampler_.setUniformInt("inputTexture", unit0, false);
        downsampler_.setUniformInt("outputImage", unit0, false);
        downsampler_.run
        (
            N_WORK_GROUPS_X(width),
            N_WORK_GROUPS_Y(height),
            N_WORK_GROUPS_Z
        );
        OpenGLWaitSync();

        //
        if (width <= minSideResolution || height <= minSideResolution)
            break;
        ++finalMipLevel;
    }

    // Dummy copy to output for visualization purposes -----------------------//
    // Bind input unit
    glActiveTexture(GL_TEXTURE0+unit0);
    glBindTexture(GL_TEXTURE_2D, bloom_->id());

    // Bind output unit
    glActiveTexture(GL_TEXTURE0+unit1);
    glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
    glBindImageTexture
    (
        unit1, 
        output_->colorBufferId(),
        0,
        GL_FALSE, 
        0, 
        GL_WRITE_ONLY,
        GL_RGBA32F
    );

    // Set uniforms
    int mip = std::min(settings.mip, finalMipLevel);
    copyToOutput_.setUniformInt("mipLevel", mip);
    copyToOutput_.setUniformInt("inputTexture", unit0, false);
    copyToOutput_.setUniformInt("outputImage", unit1, false);
    copyToOutput_.run
    (
        N_WORK_GROUPS_X(output_->width()),
        N_WORK_GROUPS_Y(output_->height()),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();
};

}