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

//----------------------------------------------------------------------------//
// Unused compute shaders

OpenGLComputeShader
    OpenGLBloomer::cachingDownsampler_
(
R"(#version 430 core
#define GROUP_SIZE 8
#define FILTER_RADIUS 2 // 13-tap downsampling algorithm has a fixed radius of 2
                                     uniform int       mip;
                                     uniform ivec2     txsz; // tx size
                                     uniform ivec2     imsz; // im size
layout(binding=0)                    uniform sampler2D tx;
layout(rgba32f, binding=0) writeonly uniform image2D   im;
layout(local_size_x=GROUP_SIZE, local_size_y=GROUP_SIZE, local_size_z = 1) in;
shared vec4  data[(GROUP_SIZE+2*FILTER_RADIUS)*(GROUP_SIZE+2*FILTER_RADIUS)];
       ivec2 gtexel    = ivec2(gl_GlobalInvocationID.xy);
       ivec2 ltexel    = ivec2(gl_LocalInvocationID.xy);
       vec2  uv        = (gtexel+.5f)/imsz;
       vec2  texelSize = 1.f / txsz;
       ivec2 tileSize  = ivec2(min(8, imsz.x), min(8, imsz.y));
int dataIndex(int i, int j)
{
    return (2*FILTER_RADIUS+tileSize.x)*(2+ltexel.y+j)+(2+ltexel.x+i);
}
void cacheData()
{
    ivec2 edge;
    edge.x = ltexel.x == 0 ? -1 : (ltexel.x == tileSize.x-1 ? 1 : 0);
    edge.y = ltexel.y == 0 ? -1 : (ltexel.y == tileSize.y-1 ? 1 : 0);
    ivec2 ij;
    for (int i=0; i<=FILTER_RADIUS*abs(edge.x); i++)
    {
        ij.x = i*edge.x;
        for (int j=0; j<=FILTER_RADIUS*abs(edge.y); j++)
        {
            ij.y = j*edge.y;
            data[dataIndex(ij.x, ij.y)] = 
                textureLod(tx, vec2(uv + vec2(ij)*texelSize), mip);
        }
    }
}
vec4 downsample()
{
    // Take 13 samples around current texel 'E' from cached data:
    // a - b - c
    // - j - k -
    // d - E - f
    // - l - m -
    // g - h - i
    vec4 a = data[dataIndex(-2, 2)];
    vec4 b = data[dataIndex( 0, 2)];
    vec4 c = data[dataIndex( 2, 2)];
    vec4 d = data[dataIndex(-2, 0)];
    vec4 E = data[dataIndex( 0, 0)];
    vec4 f = data[dataIndex( 2, 0)];
    vec4 g = data[dataIndex(-2,-2)];
    vec4 h = data[dataIndex( 0,-2)];
    vec4 i = data[dataIndex( 2,-2)];
    vec4 j = data[dataIndex(-1, 1)];
    vec4 k = data[dataIndex( 1, 1)];
    vec4 l = data[dataIndex(-1,-1)];
    vec4 m = data[dataIndex( 1,-1)];
    vec4 color = E*0.125;
    color += (a+c+g+i)*0.03125;
    color += (b+d+f+h)*0.0625;
    color += (j+k+l+m)*0.125;
    return color;
}
void main()
{
    if (gtexel.x < imsz.x && gtexel.y < imsz.y)
    {
        cacheData();
        memoryBarrierShared();
        barrier();
        imageStore(im, gtexel, downsample());
    }
})"
);

//----------------------------------------------------------------------------//
// Used compute shaders

OpenGLComputeShader
    OpenGLBloomer::brightnessMask_
    (
R"(#version 430 core
uniform float tr; // threshold
uniform float kn; // knee
uniform ivec2 sz; // tx size
layout(binding=0)                    uniform sampler2D tx;
layout(rgba32f, binding=1) writeonly uniform image2D   im;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x < sz.x && texel.y < sz.y)
    {
        vec2 uv = vec2(texel)/sz;
        vec4 col = textureLod(tx, uv, 0);
        float lmn = dot(col.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
        float f = 1.f;
        float tmk = tr-kn;
        if (lmn <= tmk)
            f = 0.f;
        else if (lmn < tr)
        {
            f = (lmn-tmk)/kn;
            f *= f*(3.f-2.f*f);
        }
        imageStore(im, texel, vec4(f*col.rgb, col.a));
    }
})"
    );

OpenGLComputeShader
    OpenGLBloomer::downsampler_
    (
R"(#version 430 core
uniform int   mip;
uniform ivec2 txsz; // tx size
uniform ivec2 imsz; // im size
// uniform bool  kr; // Karis average on/off
layout(binding=0)                    uniform sampler2D tx;
layout(rgba32f, binding=0) writeonly uniform image2D   im;
layout(local_size_x=8, local_size_y=8, local_size_z = 1) in;
ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
float karisFactor(vec3 col)
{
    return 1.f/(1.f + dot(col, vec3(0.2126f, 0.7152f, 0.0722f)));
}
vec4 downsample()
{
    vec2 texelSize = 1.f / txsz;
    float x = texelSize.x;
    float y = texelSize.y;
    vec2 uv = (texel+.5f)/imsz;
    // Take 13 samples around current texel 'E':
    // a - b - c
    // - j - k -
    // d - E - f
    // - l - m -
    // g - h - i
    vec4 a = textureLod(tx, vec2(uv.x -2*x, uv.y +2*y), mip);
    vec4 b = textureLod(tx, vec2(uv.x,      uv.y +2*y), mip);
    vec4 c = textureLod(tx, vec2(uv.x +2*x, uv.y +2*y), mip);
    vec4 d = textureLod(tx, vec2(uv.x -2*x, uv.y     ), mip);
    vec4 E = textureLod(tx, vec2(uv.x,      uv.y     ), mip);
    vec4 f = textureLod(tx, vec2(uv.x +2*x, uv.y     ), mip);
    vec4 g = textureLod(tx, vec2(uv.x -2*x, uv.y -2*y), mip);
    vec4 h = textureLod(tx, vec2(uv.x,      uv.y -2*y), mip);
    vec4 i = textureLod(tx, vec2(uv.x +2*x, uv.y -2*y), mip);
    vec4 j = textureLod(tx, vec2(uv.x   -x, uv.y   +y), mip);
    vec4 k = textureLod(tx, vec2(uv.x   +x, uv.y   +y), mip);
    vec4 l = textureLod(tx, vec2(uv.x   -x, uv.y   -y), mip);
    vec4 m = textureLod(tx, vec2(uv.x   +x, uv.y   -y), mip);
    vec4 color = 0.125f*E;
    color += 0.03125f*(a+c+g+i);
    color += 0.0625f*(b+d+f+h);
    color += 0.125f*(j+k+l+m);
    return color;
    /*
        vec4 color = vec4(0);
        if (!kr) // No Karis average
        {
            color += 0.125f  * E;
            color += 0.03125f*(a+c+g+i);
            color += 0.0625f *(b+d+f+h);
            color += 0.125f  *(j+k+l+m);
        }
        else // Karis average (sort-of)
        {
            vec4 tmp = (a+b+d+E)/4;
            color += 0.125f*karisFactor(tmp.rgb)*tmp;
            tmp =      (b+c+E+f)/4;
            color += 0.125f*karisFactor(tmp.rgb)*tmp;
            tmp =      (d+E+g+h)/4;
            color += 0.125f*karisFactor(tmp.rgb)*tmp;
            tmp =      (E+f+h+i)/4;
            color += 0.125f*karisFactor(tmp.rgb)*tmp;
            tmp =      (j+k+l+m)/4;
            color += 0.5f*karisFactor(tmp.rgb)*tmp;
        }
        return color;
    */
}
void main()
{
    if (texel.x < imsz.x && texel.y < imsz.y)
    {
        imageStore(im, texel, downsample());
    }
})"
    );

OpenGLComputeShader
    OpenGLBloomer::upsampler_
    (
R"(#version 430 core
uniform int   mip;
uniform ivec2 txsz; // tx size
uniform ivec2 imsz; // im size
uniform float ii;
uniform float hz;
uniform int   tm;
uniform float tma;
layout(binding=0)          uniform sampler2D tx;
layout(rgba32f, binding=0) uniform image2D   im;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
vec4 upsample()
{
    vec2 texelSize = 1.5 / txsz;
    float x = texelSize.x;
    float y = texelSize.y;
    vec2 uv = (texel+.5f)/imsz;
    // Take 9 samples around current texel 'E':
    // a - b - c
    // d - E - f
    // g - h - i
    vec4 a = textureLod(tx, vec2(uv.x - x, uv.y + y), mip);
    vec4 b = textureLod(tx, vec2(uv.x,     uv.y + y), mip);
    vec4 c = textureLod(tx, vec2(uv.x + x, uv.y + y), mip);
    vec4 d = textureLod(tx, vec2(uv.x - x, uv.y), mip);
    vec4 E = textureLod(tx, vec2(uv.x,     uv.y), mip);
    vec4 f = textureLod(tx, vec2(uv.x + x, uv.y), mip);
    vec4 g = textureLod(tx, vec2(uv.x - x, uv.y - y), mip);
    vec4 h = textureLod(tx, vec2(uv.x,     uv.y - y), mip);
    vec4 i = textureLod(tx, vec2(uv.x + x, uv.y - y), mip);
    vec4 color = 4* E;
    color     += 2*(b+d+f+h);
    color     +=   (a+c+g+i);
    color     *= 1.f / 16;
    return color;
}
/*vec3 tonemapRadman(vec3 c)
{
    float l = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
    return tma*c*l/(1.f+tma*l);
}*/
vec3 tonemapReinhard(vec3 c)
{
    float l = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
    return c*(1.f + l/tma)/(1.f + l);
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
    if (texel.x < imsz.x && texel.y < imsz.y)
    {
        vec4 col = upsample();
        col.rgb += imageLoad(im, texel).rgb;
        if (mip == 1) // i.e., final upsamling pass
        {
            if (hz > 0) // Apply haze
            {
                float lmn = dot(col.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
                col.rgb *= 1.0/(hz*lmn+1.0);
            }
            col.rgb *= ii;    // Apply intensity
            if (tm == 1) // Apply tone map
            {
                col.rgb = tonemapReinhard(col.rgb);
            }
            else if (tm == 2)
            {
                col.rgb = tonemapACES(col.rgb);
            }
        }
        imageStore(im, texel, col);
    }
}
)"
    );

OpenGLComputeShader
    OpenGLBloomer::adderSF32_
    (
R"(#version 430 core
uniform ivec2 sz;
layout(rgba32f, binding=0) readonly  uniform image2D imi;
layout(rgba32f, binding=1) readonly  uniform image2D imb;
layout(rgba32f, binding=2) writeonly uniform image2D imo;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x < sz.x && texel.y < sz.y)
    {
        vec4 col = imageLoad(imi, texel);
        col.rgb += imageLoad(imb, texel).rgb;
        imageStore(imo, texel, col);
    }
}
)"
    );

OpenGLComputeShader
    OpenGLBloomer::adderUI8_
    (
R"(#version 430 core
uniform ivec2 sz;
layout(rgba8ui, binding=0) readonly  uniform uimage2D imi;
layout(rgba32f, binding=1) readonly  uniform image2D  imb;
layout(rgba8ui, binding=2) writeonly uniform uimage2D imo;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x < sz.x && texel.y < sz.y)
    {
        uvec4 col = imageLoad(imi, texel);
        col.rgb += uvec3(255*imageLoad(imb, texel).rgb);
        imageStore(imo, texel, col);
    }
}
)"
    );
    
//
OpenGLBloomer::OpenGLBloomer() :
bloom_(nullptr)
{
    if (computeShaderStagesCompiled_)
        return;
    brightnessMask_.compile();
    downsampler_.compile();
    upsampler_.compile();
    adderSF32_.compile();
    adderUI8_.compile();
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
    Settings& settings
)
{
    // Size output to match input
    prepareOutput(input);
    bool sizeChanged(false);
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
        sizeChanged = true;
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
        sizeChanged = true;
    }
    static glm::ivec2 mipSize[Bloomer::maxMipDepth];
    mipSize[0] = glm::ivec2(input->width(), input->height());
    bool isSF32(input->colorBufferInternalFormat()==InternalFormat::RGBA_SF_32);

    // Bindings --------------------------------------------------------------//
    static const unsigned int inputUnit = 0;
    static const unsigned int outputUnit = 1;
    static const unsigned int bloomUnit = 2;
    glActiveTexture(GL_TEXTURE0+inputUnit);
    glBindTexture(GL_TEXTURE_2D, input->colorBufferId());
    glActiveTexture(GL_TEXTURE0+outputUnit);
    glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
    glActiveTexture(GL_TEXTURE0+bloomUnit);
    glBindTexture(GL_TEXTURE_2D, bloom_->id());
    brightnessMask_.setUniformInt("tx", inputUnit);
    brightnessMask_.setUniformInt("im", bloomUnit, false);
    downsampler_.setUniformInt("tx", bloomUnit);
    downsampler_.setUniformInt("im", bloomUnit, false);
    upsampler_.setUniformInt("tx", bloomUnit);
    upsampler_.setUniformInt("im", bloomUnit, false);
    OpenGLComputeShader* adder_ = isSF32 ? &adderSF32_ : &adderUI8_;
    adder_->setUniformInt("imi", inputUnit);
    adder_->setUniformInt("imb", bloomUnit, false);
    adder_->setUniformInt("imo", outputUnit, false);

    // Macros
#define N_WORK_GROUPS_X(x) std::ceil(float(x)/8)
#define N_WORK_GROUPS_Y(y) std::ceil(float(y)/8)
#define N_WORK_GROUPS_Z 1

    // Brightness mask -------------------------------------------------------//
    glBindImageTexture
    (
        bloomUnit, 
        bloom_->id(), 
        0,
        GL_FALSE, 
        0, 
        GL_WRITE_ONLY,
        GL_RGBA32F
    );
    brightnessMask_.setUniformFloat("tr", settings.threshold);
    brightnessMask_.setUniformFloat("kn", settings.knee, false);
    brightnessMask_.setUniformInt2("sz", mipSize[0], false);
    brightnessMask_.run
    (
        N_WORK_GROUPS_X(mipSize[0].x),
        N_WORK_GROUPS_Y(mipSize[0].y),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();

    // Downsampling ----------------------------------------------------------//
    int minSideResolution = 2; // Could be passed via settings
    int mipLevel = 1;
    bool firstLevel = true;
    while (true)
    {
        glBindImageTexture
        (
            bloomUnit, 
            bloom_->id(), 
            mipLevel,
            GL_FALSE, 
            0, 
            GL_WRITE_ONLY,
            GL_RGBA32F
        );
        GLint width, height;
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D,
            mipLevel,
            GL_TEXTURE_WIDTH, 
            &width
        );
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D, 
            mipLevel,
            GL_TEXTURE_HEIGHT, 
            &height
        );
        glm::ivec2& size = mipSize[mipLevel];
        size.x = width;
        size.y = height;
        downsampler_.setUniformInt("mip", mipLevel-1);
        downsampler_.setUniformInt2("txsz", mipSize[mipLevel-1], false);
        downsampler_.setUniformInt2("imsz", size, false);
        //downsampler_.setUniformInt("kr", int(firstLevel), false);
        downsampler_.run
        (
            N_WORK_GROUPS_X(width),
            N_WORK_GROUPS_Y(height),
            N_WORK_GROUPS_Z
        );
        OpenGLWaitSync();
        if 
        (
            width <= minSideResolution || 
            height <= minSideResolution ||
            mipLevel >= settings.mipDepth
        )
            break;
        ++mipLevel;
        if (firstLevel)
            firstLevel = false;
    }
    int lastMipLevel = mipLevel;
    if (sizeChanged)
        settings.mipDepth = lastMipLevel;

    // Upsampling ------------------------------------------------------------//
    while(mipLevel > 0)
    {
        glBindImageTexture
        (
            bloomUnit, 
            bloom_->id(), 
            mipLevel-1,
            GL_FALSE, 
            0, 
            GL_READ_WRITE,
            GL_RGBA32F
        );
        upsampler_.setUniformInt("mip", mipLevel);
        upsampler_.setUniformInt2("txsz", mipSize[mipLevel], false);
        const glm::ivec2& size = mipSize[mipLevel-1];
        upsampler_.setUniformInt2("imsz", size, false);
        upsampler_.setUniformFloat("ii", settings.intensity, false);
        float hz = settings.haze;
        if (hz > 0.f)
            hz *= std::pow(2, hz);
        upsampler_.setUniformFloat("hz", hz, false);
        upsampler_.setUniformInt("tm", (int)(settings.toneMap), false);
        if (settings.toneMap == ToneMap::Reinhard)
        {
            float tma = 
                settings.reinhardWhitePoint*
                settings.reinhardWhitePoint;
            upsampler_.setUniformFloat("tma", tma, false);
        }
        /*switch (settings.toneMap)
        {
            case ToneMap::Radman :
            {
                float tma = settings.radmanExposure;
                if (tma > 0.f)
                    tma *= std::pow(2, tma);
                upsampler_.setUniformFloat("tma", tma, false);
                break;
            }
            case ToneMap::Reinhard :
            {
                float tma = 
                    settings.reinhardWhitePoint*
                    settings.reinhardWhitePoint;
                upsampler_.setUniformFloat("tma", tma, false);
                break;
            }
            default:
                break;
        }*/
        upsampler_.run
        (
            N_WORK_GROUPS_X(size.x),
            N_WORK_GROUPS_Y(size.y),
            N_WORK_GROUPS_Z
        );
        OpenGLWaitSync();
        --mipLevel;
    }

    // Add bloom to input ----------------------------------------------------//
    glBindImageTexture
    (
        inputUnit, 
        input->colorBufferId(), 
        0,
        GL_FALSE, 
        0, 
        GL_READ_ONLY,
        isSF32 ? GL_RGBA32F : GL_RGBA8UI
    );
    glBindImageTexture
    (
        bloomUnit, 
        bloom_->id(), 
        0,
        GL_FALSE, 
        0, 
        GL_READ_ONLY,
        GL_RGBA32F
    );
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
    const glm::ivec2& size = mipSize[0];
    adder_->setUniformInt2("sz", size);
    adder_->run
    (
        N_WORK_GROUPS_X(size.x),
        N_WORK_GROUPS_Y(size.y),
        N_WORK_GROUPS_Z
    );
    OpenGLWaitSync();
};

}