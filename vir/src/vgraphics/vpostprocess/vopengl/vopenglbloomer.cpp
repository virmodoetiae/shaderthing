#include "vpch.h"
#include "vgraphics/vpostprocess/vopengl/vopenglbloomer.h"
#include "vgraphics/vcore/vopengl/vopenglmisc.h"

namespace vir
{

typedef TextureBuffer::InternalFormat InternalFormat;
typedef TextureBuffer::FilterMode FilterMode;
typedef TextureBuffer::WrapMode WrapMode;
typedef Bloomer::Settings::ToneMap ToneMap;

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
    vec4 a = textureLod(tx, vec2(uv.x -2*x, uv.y +2*y), mip);
    vec4 b = textureLod(tx, vec2(uv.x,      uv.y +2*y), mip);
    vec4 c = textureLod(tx, vec2(uv.x +2*x, uv.y +2*y), mip);
    vec4 d = textureLod(tx, vec2(uv.x -2*x, uv.y     ), mip);
    vec4 e = textureLod(tx, vec2(uv.x,      uv.y     ), mip);
    vec4 f = textureLod(tx, vec2(uv.x +2*x, uv.y     ), mip);
    vec4 g = textureLod(tx, vec2(uv.x -2*x, uv.y -2*y), mip);
    vec4 h = textureLod(tx, vec2(uv.x,      uv.y -2*y), mip);
    vec4 i = textureLod(tx, vec2(uv.x +2*x, uv.y -2*y), mip);
    vec4 j = textureLod(tx, vec2(uv.x   -x, uv.y   +y), mip);
    vec4 k = textureLod(tx, vec2(uv.x   +x, uv.y   +y), mip);
    vec4 l = textureLod(tx, vec2(uv.x   -x, uv.y   -y), mip);
    vec4 m = textureLod(tx, vec2(uv.x   +x, uv.y   -y), mip);
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
    OpenGLBloomer::upsampler_
    (
R"(#version 430 core
uniform int mipLevel;
uniform int toneMap;
uniform float intensity;
uniform float toneMapArg;
layout(binding=0) uniform sampler2D inputTexture;
layout(rgba32f, binding=0) uniform image2D outputImage;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
vec4 upsample(sampler2D tx, vec2 tc, int mip)
{
    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    vec2 texelSize = 1.0 / textureSize(tx, mip);
    float x = texelSize.x;
    float y = texelSize.y;

    // Take 9 samples around current texel 'e':
    // a - b - c
    // d - e - f
    // g - h - i
    vec4 a = textureLod(tx, vec2(tc.x - x, tc.y + y), mip);
    vec4 b = textureLod(tx, vec2(tc.x,     tc.y + y), mip);
    vec4 c = textureLod(tx, vec2(tc.x + x, tc.y + y), mip);
    vec4 d = textureLod(tx, vec2(tc.x - x, tc.y), mip);
    vec4 e = textureLod(tx, vec2(tc.x,     tc.y), mip);
    vec4 f = textureLod(tx, vec2(tc.x + x, tc.y), mip);
    vec4 g = textureLod(tx, vec2(tc.x - x, tc.y - y), mip);
    vec4 h = textureLod(tx, vec2(tc.x,     tc.y - y), mip);
    vec4 i = textureLod(tx, vec2(tc.x + x, tc.y - y), mip);

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    vec4 color = e*4.0;
    color += (b+d+f+h)*2.0;
    color += (a+c+g+i);
    color *= 1.0 / 16.0;
    return color;
}
vec3 tonemapReinhard(vec3 c, float ex)
{
    float l = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
    return ex*c*l/(1.f + l);
}
vec3 tonemapReinhardExtended(vec3 c, float wp)
{
    float l = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
    return c*(1.f + l/wp/wp)/(1.f + l);
}
vec3 tonemapACES(vec3 v)
{
    v = vec3(
        .59719f*v.x + .35458f*v.y + .04823f*v.z,
        .07600f*v.x + .90834f*v.y + .01566f*v.z,
        .02840f*v.x + .13383f*v.y + .83777f*v.z
    );
    v = (v*(v + .0245786f) - .000090537f)/
        (v*(.983729f*v + .4329510f) + .238081f);
    return vec3(
        1.60475f*v.x + -.53108f*v.y + -.07367f*v.z,
        -.10208f*v.x + 1.10813f*v.y + -.00605f*v.z,
        -.00327f*v.x + -.07276f*v.y + 1.07602f*v.z
    );
}
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    if (texel.x < size.x && texel.y < size.y)
    {
        vec2 uv = (vec2(texel)+.5f)/size;
        vec4 col = upsample(inputTexture, uv, mipLevel);
        col.rgb += imageLoad(outputImage, texel).rgb;
        if (mipLevel == 1) // i.e., final upsamling pass
        {
            // if toneMap == 0 then no tone map is used
            if (toneMap == 1)
            {
                col.rgb = tonemapReinhard(col.rgb, toneMapArg);
            }
            else if (toneMap == 2)
            {
                col.rgb = tonemapReinhardExtended(col.rgb, toneMapArg);
            }
            else if (toneMap == 3)
            {
                col.rgb = tonemapACES(col.rgb);
            }
            col.rgb *= intensity;
        }
        imageStore(outputImage, texel, col);
    }
}
)"
    );

OpenGLComputeShader
    OpenGLBloomer::adder_
    (
R"(#version 430 core
layout(rgba32f, binding=0) readonly uniform image2D inputImage;
layout(rgba32f, binding=1) readonly uniform image2D bloomImage;
layout(rgba32f, binding=2) writeonly uniform image2D outputImage;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    if (texel.x < size.x && texel.y < size.y)
    {
        vec4 col = imageLoad(inputImage, texel);
        col.rgb += imageLoad(bloomImage, texel).rgb;
        imageStore(outputImage, texel, col);
    }
}
)"
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
    upsampler_.compile();
    adder_.compile();
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
    int minSideResolution = 2; // Could be passed via settings
    int mipLevel=1;
    static GLint mipWidths[16];
    static GLint mipHeights[16];
    mipWidths[0] = input->width();
    mipHeights[0] = input->height();
    while (true)
    {
        // Bind output target mip level image
        glBindImageTexture
        (
            unit0, 
            bloom_->id(), 
            mipLevel,
            GL_FALSE, 
            0, 
            GL_WRITE_ONLY,
            GL_RGBA32F
        );

        // Get target mip level texture size
        GLint* width = &mipWidths[mipLevel];
        GLint* height = &mipHeights[mipLevel];
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D,
            mipLevel,
            GL_TEXTURE_WIDTH, 
            width
        );
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D, 
            mipLevel,
            GL_TEXTURE_HEIGHT, 
            height
        );

        //std::cout<<width<<" "<<height<<" "<<finalMipLevel<<std::endl;

        downsampler_.setUniformInt("mipLevel", mipLevel-1);
        downsampler_.setUniformInt("inputTexture", unit0, false);
        downsampler_.setUniformInt("outputImage", unit0, false);
        downsampler_.run
        (
            N_WORK_GROUPS_X(*width),
            N_WORK_GROUPS_Y(*height),
            N_WORK_GROUPS_Z
        );
        OpenGLWaitSync();

        //
        if (*width <= minSideResolution || *height <= minSideResolution)
            break;
        ++mipLevel;
    }
    int maxMipLevel = mipLevel;

    // Upsampling ------------------------------------------------------------//
    glActiveTexture(GL_TEXTURE0+unit0);
    glBindTexture(GL_TEXTURE_2D, bloom_->id());
    while(mipLevel > 0)
    {
        // Bind output target mip level image
        glBindImageTexture
        (
            unit0, 
            bloom_->id(), 
            mipLevel-1,
            GL_FALSE, 
            0, 
            GL_READ_WRITE,
            GL_RGBA32F
        );
        upsampler_.setUniformInt("mipLevel", mipLevel);
        upsampler_.setUniformInt("toneMap", (int)(settings.toneMap));
        switch (settings.toneMap)
        {
            case ToneMap::Reinhard :
                upsampler_.setUniformFloat
                (
                    "toneMapArg", 
                    settings.reinhardExposure
                );
                break;
            case ToneMap::ReinhardExtended :
                upsampler_.setUniformFloat
                (
                    "toneMapArg", 
                    settings.reinhardWhitePoint
                );
                break;
            default:
                break;
        }
        upsampler_.setUniformFloat("intensity", settings.intensity);
        upsampler_.setUniformInt("inputTexture", unit0);
        upsampler_.setUniformInt("outputImage", unit0);
        upsampler_.run
        (
            N_WORK_GROUPS_X(mipWidths[mipLevel-1]),
            N_WORK_GROUPS_Y(mipHeights[mipLevel-1]),
            N_WORK_GROUPS_Z
        );
        OpenGLWaitSync();
        --mipLevel;
    }

    // Add bloom to input ----------------------------------------------------//
    glActiveTexture(GL_TEXTURE0+unit0);
    glBindTexture(GL_TEXTURE_2D, input->colorBufferId());
    glBindImageTexture
    (
        unit0, 
        input->colorBufferId(), 
        0,
        GL_FALSE, 
        0, 
        GL_READ_ONLY,
        GL_RGBA32F
    );
    glActiveTexture(GL_TEXTURE0+unit1);
    glBindTexture(GL_TEXTURE_2D, bloom_->id());
    glBindImageTexture
    (
        unit1, 
        bloom_->id(), 
        0,
        GL_FALSE, 
        0, 
        GL_READ_ONLY,
        GL_RGBA32F
    );
    glActiveTexture(GL_TEXTURE0+2);
    glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
    glBindImageTexture
    (
        2, 
        output_->colorBufferId(), 
        0,
        GL_FALSE, 
        0, 
        GL_WRITE_ONLY,
        GL_RGBA32F
    );
    adder_.setUniformInt("inputImage", unit0);
    adder_.setUniformInt("bloomImage", unit1);
    adder_.setUniformInt("outputImage", 2);
    adder_.run
    (
        N_WORK_GROUPS_X(mipWidths[0]),
        N_WORK_GROUPS_Y(mipHeights[0]),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();

    /*
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
    int mip = std::min(settings.mip, maxMipLevel);
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
    */
};

}