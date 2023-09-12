#include "vpch.h"
#include <cmath>
#include "vgraphics/vmisc/vopengl/vopenglkmeansquantizer.h"

namespace vir
{

// ComputeShaderStage-related ------------------------------------------------//

OpenGLKMeansQuantizer::ComputeShaderStage::~ComputeShaderStage()
{
    glDeleteProgram(id_);
}

void OpenGLKMeansQuantizer::ComputeShaderStage::compile()
{
    // Compile shader
    const char* sourceCstr = source_.c_str();
    GLuint computeShader0 = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader0, 1, &sourceCstr, NULL);
    glCompileShader(computeShader0);
    GLint logLength0 = 0;
    glGetShaderiv(computeShader0, GL_INFO_LOG_LENGTH, &logLength0);
    if (logLength0 > 0)
    {
        std::vector<GLchar> logv(logLength0);
        glGetShaderInfoLog(computeShader0, logLength0, &logLength0, &logv[0]);
        std::string log(logv.begin(), logv.end());
        glDeleteShader(computeShader0);
        std::cout << log << std::endl;
        throw std::runtime_error("[CS0] "+log);
    }
    id_ = glCreateProgram();
    glAttachShader(id_, computeShader0);
    glLinkProgram(id_);
    GLint logLength = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        std::vector<GLchar> logv(logLength);
        glGetProgramInfoLog(id_, logLength, &logLength, &logv[0]);
        std::string log(logv.begin(), logv.end());
        glDeleteProgram(id_);
        std::cout << log << std::endl;
        throw std::runtime_error("[CSP] "+log);
    }
    glDeleteShader(computeShader0);
}

GLint OpenGLKMeansQuantizer::ComputeShaderStage::getUniformLocation
(
    std::string& uniformName
)
{
    GLint location;
    if (uniformLocations_.find(uniformName) != uniformLocations_.end())
        location = (GLint)(uniformLocations_.at(uniformName));
    else
    {
        location = glGetUniformLocation(id_, uniformName.c_str());
        if (location != -1)
            uniformLocations_[uniformName] = location;
    }
    return location;
}

void OpenGLKMeansQuantizer::ComputeShaderStage::setUniformInt
(
    std::string uniformName, 
    int value,
    bool autoUse
)
{
    if (autoUse)
        use();
    glUniform1i(getUniformLocation(uniformName), value);
}

void OpenGLKMeansQuantizer::ComputeShaderStage::setUniformFloat
(
    std::string uniformName, 
    float value,
    bool autoUse
)
{
    if (autoUse)
        use();
    glUniform1f(getUniformLocation(uniformName), value);
}

void OpenGLKMeansQuantizer::ComputeShaderStage::use()
{
    glUseProgram(id_);
}

void OpenGLKMeansQuantizer::ComputeShaderStage::run
(
    int x, 
    int y, 
    int z, 
    GLbitfield barriers
)
{
    glDispatchCompute(x, y, z);
    glMemoryBarrier(barriers);
}

// OpenGLKMeanQuantizer static data ------------------------------------------//

bool OpenGLKMeansQuantizer::computeShaderStagesCompiled = false;

OpenGLKMeansQuantizer::ComputeShaderStage 
    OpenGLKMeansQuantizer::computeShader_findMaxSqrDistCol
    (
R"(#version 460 core
uniform int counter;
uniform int paletteSize;
layout(binding=0) uniform atomic_uint clusteringError;
layout(rgba8ui, binding=0) uniform uimage2D image;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 d2MaxPos = ivec2(paletteSize+1,2);
    if (counter == 0)
    {
        if (gid == ivec2(0,0))
        {
            uvec4 img = imageLoad(image, gid);
            for (int i=0; i<3*paletteSize; i++)
            {
                for (int j=0; j<3; j++)
                {
                    imageAtomicExchange(paletteData, ivec2(i,j), 0);
                }
            }
            imageAtomicExchange(paletteData, ivec2(0, 0), img.r);
            imageAtomicExchange(paletteData, ivec2(1, 0), img.g);
            imageAtomicExchange(paletteData, ivec2(2, 0), img.b);
            imageAtomicExchange(paletteData, d2MaxPos, 0);
            atomicCounterExchange(clusteringError, 0);
        }
        return;
    }
    uint d2m = 195075;
    ivec3 img = ivec3(imageLoad(image, gid).rgb);
    for (int i=0; i<counter; i++)
    {
        uint pr = imageLoad(paletteData, ivec2(3*i, 0)).r;
        uint pg = imageLoad(paletteData, ivec2(3*i+1, 0)).r;
        uint pb = imageLoad(paletteData, ivec2(3*i+2, 0)).r;
        ivec3 dif = img-ivec3(pr,pg,pb);
        uint d2 = dif.x*dif.x + dif.y*dif.y + dif.z*dif.z;
        if (d2 < d2m)
        {
            d2m = d2;
        }
    }
    uint d2m0 = imageAtomicMax(paletteData, d2MaxPos, d2m);
    if (d2m0 < d2m)
    {
        imageAtomicExchange(paletteData, ivec2(paletteSize+2,2), uint(gid.x));
        imageAtomicExchange(paletteData, ivec2(paletteSize+3,2), uint(gid.y));
    }
})" );

OpenGLKMeansQuantizer::ComputeShaderStage 
    OpenGLKMeansQuantizer::computeShader_setNextPaletteCol
    (
R"(#version 460 core
uniform int counter;
uniform int paletteSize;
layout(rgba8ui, binding=0) uniform uimage2D image;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    if (counter == 0)
        return;
    uint x = imageLoad(paletteData, ivec2(paletteSize+2,2)).r;
    uint y = imageLoad(paletteData, ivec2(paletteSize+3,2)).r;
    uvec4 img = imageLoad(image, ivec2(x,y));
    imageStore(paletteData, ivec2(3*counter,0), uvec4(img.r,0,0,1));
    imageStore(paletteData, ivec2(3*counter+1,0), uvec4(img.g,0,0,1));
    imageStore(paletteData, ivec2(3*counter+2,0), uvec4(img.b,0,0,1));
    ivec2 d2MaxPos = ivec2(paletteSize+1,2);
    imageStore(paletteData, d2MaxPos, uvec4(0,0,0,0));
})" );

OpenGLKMeansQuantizer::ComputeShaderStage 
    OpenGLKMeansQuantizer::computeShader_buildClustersFromPalette
    (
R"(#version 460 core
uniform int paletteSize;
layout(binding=0) uniform atomic_uint clusteringError;
layout(rgba8ui, binding=1) uniform uimage2D image;
layout(r32ui, binding=2) uniform uimage2D paletteData;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() 
{ 
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    uvec4 img = uvec4(imageLoad(image, gid));
    int d2m = 195075; // max d2 is 3*255*255 = 195075
    int index = 0;
    for (int i=0; i<paletteSize; i++)
    {
        ivec3 d = ivec3(img);
        d.x -= int(imageLoad(paletteData, ivec2(i*3,   0)).r);
        d.y -= int(imageLoad(paletteData, ivec2(i*3+1, 0)).r);
        d.z -= int(imageLoad(paletteData, ivec2(i*3+2, 0)).r);
        int d2 = d.x*d.x + d.y*d.y + d.z*d.z;
        if (d2 < d2m)
        {
            d2m = d2;
            index = i;
        }
    }
    imageAtomicAdd(paletteData, ivec2(index*3,   1), img.r);
    imageAtomicAdd(paletteData, ivec2(index*3+1, 1), img.g);
    imageAtomicAdd(paletteData, ivec2(index*3+2, 1), img.b);
    imageAtomicAdd(paletteData, ivec2(index,     2), 1);
    atomicCounterAdd(clusteringError, uint(d2m));
})" );

OpenGLKMeansQuantizer::ComputeShaderStage 
    OpenGLKMeansQuantizer::computeShader_updatePaletteFromClusters
    (
R"(#version 460 core
layout(binding=0) uniform atomic_uint clusteringError;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(rgba8ui, binding=2) uniform uimage2D image;
uniform int imageWidth;
uniform int imageHeight;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
float rand(vec2 seed)
{
	return fract(sin(dot(seed.xy, vec2(12.9898, 78.233)))*43758.5453);
}
void main() 
{ 
    uint gid = gl_GlobalInvocationID.x;
    // Read & clear accumulators for next counter loop iteration
    uint r = imageAtomicExchange(paletteData, ivec2(gid*3, 1), 0);
    uint g = imageAtomicExchange(paletteData, ivec2(gid*3+1, 1), 0);
    uint b = imageAtomicExchange(paletteData, ivec2(gid*3+2, 1), 0);
    uint count = imageAtomicExchange(paletteData, ivec2(gid, 2), 0);
    float error = float(atomicCounterExchange(clusteringError, 0));
    if (count == 0)
    {
        count = 1;
        float w = imageWidth - 1;
        float h = imageHeight - 1;
        int x = int(rand(vec2(error/w, h*(1+gid)))*w);
        int y = int(rand(vec2(w*(1+gid), error/h))*h);
        uvec4 col = imageLoad(image, ivec2(x,y));
        r = col.r;
        g = col.g;
        b = col.b;
    }
    imageAtomicExchange(paletteData, ivec2(gid*3,   0), uint(float(r)/count));
    imageAtomicExchange(paletteData, ivec2(gid*3+1, 0), uint(float(g)/count));
    imageAtomicExchange(paletteData, ivec2(gid*3+2, 0), uint(float(b)/count));
    
})" );

OpenGLKMeansQuantizer::ComputeShaderStage 
    OpenGLKMeansQuantizer::computeShader_quantizeInput
    (
R"(#version 460 core
uniform int paletteSize;
uniform int ditherLevel;
uniform float ditherThreshold;
uniform int alphaCutoff;
uniform bool computeDelta;
layout(rgba8ui, binding=0) uniform uimage2D image;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(r8ui, binding=2) uniform uimage2D indexedImage;
layout(rgba8ui, binding=3) uniform uimage2D oldQuantizedImage;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

const float ditherMask2x2[4] = float[4]
(
    0.0 /4.0-3.0/8.0,
    2.0 /4.0-3.0/8.0,

    3.0 /4.0-3.0/8.0,
    1.0 /4.0-3.0/8.0
);
const float ditherMask4x4[16] = float[16]
(
    0.0 /16.0-15.0/32.0,
    8.0 /16.0-15.0/32.0,
    2.0 /16.0-15.0/32.0,
    10.0 /16.0-15.0/32.0,

    12.0 /16.0-15.0/32.0,
    4.0 /16.0-15.0/32.0,
    14.0 /16.0-15.0/32.0,
    6.0 /16.0-15.0/32.0,

    3.0 /16.0-15.0/32.0,
    11.0 /16.0-15.0/32.0,
    1.0 /16.0-15.0/32.0,
    9.0 /16.0-15.0/32.0,

    15.0 /16.0-15.0/32.0,
    7.0 /16.0-15.0/32.0,
    13.0 /16.0-15.0/32.0,
    5.0 /16.0-15.0/32.0
);

void main() 
{ 
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    uvec4 img = uvec4(imageLoad(image, gid));
    switch (ditherLevel)
    {
        case 0 :
        {
            break;
        }
        case 1 :
        {
            float maski = 256.0*ditherMask2x2[2*(gid.y%2)+(gid.x%2)];
            img = uvec4(ivec3(img.rgb)+int(maski*ditherThreshold+0.5), img.a);
            break;
        }
        case 2 :
        {
            float maski = 256.0*ditherMask4x4[4*(gid.y%4)+(gid.x%4)];
            img = uvec4(ivec3(img.rgb)+int(maski*ditherThreshold+0.5), img.a);
            break;
        }
    }
    int d2m = 195075; // max d2 is 3*255*255 = 195075
    int index = 0;
    for (int i=0; i<paletteSize; i++)
    {
        ivec3 d = ivec3(img);
        d.x -= int(imageLoad(paletteData, ivec2(i*3,   0)).r);
        d.y -= int(imageLoad(paletteData, ivec2(i*3+1, 0)).r);
        d.z -= int(imageLoad(paletteData, ivec2(i*3+2, 0)).r);
        int d2 = d.x*d.x + d.y*d.y + d.z*d.z;
        if (d2 < d2m)
        {
            d2m = d2;
            index = i;
        }
    }
    uint r = imageLoad(paletteData, ivec2(index*3,   0)).r;
    uint g = imageLoad(paletteData, ivec2(index*3+1, 0)).r;
    uint b = imageLoad(paletteData, ivec2(index*3+2, 0)).r;
    uint a = (alphaCutoff != -1) ? ((img.a >= alphaCutoff ? 255 : 0)) : img.a;
    uvec4 newColor = uvec4(r,g,b,a);
    if (computeDelta)
    {
        uvec4 oldColor = imageLoad(oldQuantizedImage, gid);
        if (oldColor.a > 0 && newColor.rgb == oldColor.rgb)
        {
            index = paletteSize;
        }
        imageStore(oldQuantizedImage, gid, newColor);
    }
    imageStore(image, gid, newColor);
    imageStore(indexedImage, gid, uvec4(index, 0, 0, 0));
})" );

//----------------------------------------------------------------------------//
// Private member functions

void OpenGLKMeansQuantizer::quantizeOpenGLTexture
(
    GLuint id,
    uint32_t width,
    uint32_t height,
    uint32_t paletteSize, 
    unsigned char* palette,
    uint32_t ditherLevel,
    bool reseedPalette,
    bool recalculatePalette,
    float relTol,
    float ditherThreshold,
    int alphaCutoff,
    bool regenerateMipmap,
    bool fastKMeans,
    bool computeDelta,
    uint32_t inputUnit
)
{
    //
    deltaComputed_ = computeDelta;

    // Limit palette size (honestly, you cannot tell a difference above 256)
    paletteSize = std::min(std::max(paletteSize,2u),(computeDelta)?255u:256u);
    
    // Bind buffers
    // Input image
    glActiveTexture(GL_TEXTURE0+inputUnit);
    glBindTexture(GL_TEXTURE_2D, id);

    uint32_t targetLevel = 0;
    int mWidth = width;
    int mHeight = height;
    if 
    (
        fastKMeans && 
        (
            paletteSize_ != paletteSize || 
            recalculatePalette
        )
    )
    {
        // Determine on which mipmap level the k-means should run
        glGenerateMipmap(GL_TEXTURE_2D);
        auto log4 = [](float x) 
        {
            static float log2f4(std::log2f(4));
            return uint32_t(ceil(std::log2f(x)/log2f4));
        };
        uint32_t maxNPixels = 128*128/log4(paletteSize);
        int maxLevel;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);
        targetLevel = std::min
        (
            log4(std::max(width*height/maxNPixels, 1u)), (uint32_t)maxLevel
        );
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D, 
            targetLevel, 
            GL_TEXTURE_WIDTH, 
            &mWidth
        );
        glGetTexLevelParameteriv
        (
            GL_TEXTURE_2D, 
            targetLevel, 
            GL_TEXTURE_HEIGHT,
            &mHeight
        );
    }
    glBindImageTexture
    (
        inputUnit, 
        id, 
        targetLevel, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        GL_RGBA8UI
    );
    
    // Palette data texture
    uint32_t paletteDataUnit = inputUnit+1;
    glActiveTexture(GL_TEXTURE0+paletteDataUnit);
    glBindTexture(GL_TEXTURE_2D, paletteData_);
    glBindImageTexture(paletteDataUnit, paletteData_, 0, GL_FALSE, 0, 
        GL_READ_WRITE, GL_R32UI);
    // Indexed data texture
    uint32_t indexedDataUnit = inputUnit+2;
    glActiveTexture(GL_TEXTURE0+indexedDataUnit);
    glBindTexture(GL_TEXTURE_2D, indexedData_);
    glBindImageTexture(indexedDataUnit, indexedData_, 0, GL_FALSE, 0, 
        GL_READ_WRITE, GL_R8UI);
    //
    uint32_t oldQuantizedInputUnit = inputUnit+3;
    // Atomic error counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, clusteringError_);
    uint32_t binding = 0;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, 
        clusteringError_);

    // Resize indexedData if necessary
    if (width_ != width || height_ != height)
    {
        width_ = width;
        height_ = height;
        int nPixels = width*height;
        glActiveTexture(GL_TEXTURE0+indexedDataUnit);
        glBindTexture(GL_TEXTURE_2D, indexedData_);
        glTexImage2D
        (
            GL_TEXTURE_2D, 
            0, 
            GL_R8UI, 
            width, 
            height, 
            0, 
            GL_RED_INTEGER, 
            GL_UNSIGNED_BYTE, 
            NULL
        );

        // Read-only
        auto PBOFlags = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | 
            GL_MAP_COHERENT_BIT;
        glDeleteBuffers(1, &indexedDataPBO_);
        glGenBuffers(1, &indexedDataPBO_);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, indexedDataPBO_);
        glBufferStorage
        (
            GL_PIXEL_PACK_BUFFER, 
            nPixels*sizeof(unsigned char), // sizeof(unsigned char) = 1...
            NULL, 
            PBOFlags
        );
        mappedIndexedData_ = glMapBufferRange
        (
            GL_PIXEL_PACK_BUFFER, 
            0, 
            nPixels*sizeof(unsigned char), // sizeof(unsigned char) = 1...
            PBOFlags
        );
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        //
        if (computeDelta)
        {
            glGenTextures(1, &oldQuantizedInput_);
            glActiveTexture(GL_TEXTURE_2D+oldQuantizedInputUnit);
            glBindTexture(GL_TEXTURE_2D, oldQuantizedInput_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexImage2D
            (
                GL_TEXTURE_2D, 
                0, 
                GL_RGBA, 
                width, 
                height, 
                0, 
                GL_RGBA, 
                GL_UNSIGNED_BYTE, 
                NULL
            );
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    if (computeDelta)
    {
        glActiveTexture(GL_TEXTURE_2D+oldQuantizedInputUnit);
        glBindTexture(GL_TEXTURE_2D, oldQuantizedInput_);
        glBindImageTexture
        (
            oldQuantizedInputUnit, 
            oldQuantizedInput_, 
            0, 
            GL_FALSE, 
            0, 
            GL_READ_WRITE, 
            GL_RGBA8UI
        );
    }

    // Resize paletteData if necessary
    if (paletteSize_ != paletteSize)
    {
        reseedPalette = true;
        recalculatePalette = true;
        paletteSize_ = paletteSize;
        glActiveTexture(GL_TEXTURE0+paletteDataUnit);
        glBindTexture(GL_TEXTURE_2D, paletteData_);
        glTexImage2D
        (
            GL_TEXTURE_2D, 
            0, 
            GL_R32UI, 
            3*paletteSize, 
            3, 
            0, 
            GL_RED_INTEGER, 
            GL_UNSIGNED_INT, 
            NULL
        );

        //
        auto PBOFlags = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | 
            GL_MAP_COHERENT_BIT;
        glDeleteBuffers(1, &paletteDataPBO_);
        glGenBuffers(1, &paletteDataPBO_);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, paletteDataPBO_);
        glBufferStorage
        (
            GL_PIXEL_PACK_BUFFER, 
            3*3*paletteSize*sizeof(uint32_t),
            NULL, 
            PBOFlags
        );
        mappedPaletteData_ = glMapBufferRange
        (
            GL_PIXEL_PACK_BUFFER, 
            0, 
            3*paletteSize_*sizeof(uint32_t), 
            PBOFlags
        );
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        //
        auto WriteOnlyPBOFlags = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | 
            GL_MAP_COHERENT_BIT;
        glDeleteBuffers(1, &paletteDataWriteOnlyPBO_);
        glGenBuffers(1, &paletteDataWriteOnlyPBO_);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, paletteDataWriteOnlyPBO_);
        glBufferStorage
        (
            GL_PIXEL_UNPACK_BUFFER, 
            3*3*paletteSize*sizeof(uint32_t),
            NULL, 
            WriteOnlyPBOFlags
        );
        mappedWriteOnlyPaletteData_ = glMapBufferRange
        (
            GL_PIXEL_UNPACK_BUFFER, 
            0, 
            3*paletteSize_*sizeof(uint32_t), 
            WriteOnlyPBOFlags
        );
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        
        // Reset uniforms
        computeShader_findMaxSqrDistCol.setUniformInt
        (
            "paletteSize", 
            paletteSize
        );
        computeShader_setNextPaletteCol.setUniformInt
        (
            "paletteSize", 
            paletteSize
        );
        computeShader_buildClustersFromPalette.setUniformInt
        (
            "paletteSize", 
            paletteSize
        );
    }

    // If provided, upload the palette to the GPU and use it instead of running
    // the KMeans
    if (palette != nullptr)
    {
        recalculatePalette = false;
        glActiveTexture(GL_TEXTURE0+paletteDataUnit);
        glBindTexture(GL_TEXTURE_2D, paletteData_);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, paletteDataWriteOnlyPBO_);
        waitSync();
        std::memcpy
        (
            mappedWriteOnlyPaletteData_, 
            (void*)palette, 
            3*paletteSize*sizeof(unsigned char)
        );
        resetSync();
        glTexSubImage2D
        (
            GL_TEXTURE_2D, 
            0, 
            0, 
            0, 
            3*paletteSize, 
            1, 
            GL_RED_INTEGER, 
            GL_UNSIGNED_BYTE,
            NULL
        );
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    if (recalculatePalette)
    {
        // Build initial palette

        // I suspect that that the largest bottleneck is this first CPU-based 
        // loop from which async OpenGL commands are issued. Is there a 
        // smarter way?
        if (reseedPalette)
        {
            for (int counter = 0; counter < paletteSize; counter++)
            {
                // Determine the pixel of the image which is the furthest away
                // (in color-space) from the last palette color in paletteData
                computeShader_findMaxSqrDistCol.setUniformInt
                (
                    "counter", 
                    counter
                );
                if (counter == 0)
                {
                    computeShader_findMaxSqrDistCol.setUniformInt
                    (
                        "image", 
                        inputUnit, 
                        false
                    );
                    computeShader_findMaxSqrDistCol.setUniformInt
                    (
                        "paletteData",
                        paletteDataUnit,
                        false
                    );
                }
                computeShader_findMaxSqrDistCol.run(mWidth, mHeight, 1);

                // Add the previously selected image pixel to the palette set,
                // reset max distance
                computeShader_setNextPaletteCol.setUniformInt
                (
                    "counter", 
                    counter
                );
                if (counter == 0)
                {
                    computeShader_setNextPaletteCol.setUniformInt
                    (
                        "image", 
                        inputUnit, 
                        false
                    );
                    computeShader_setNextPaletteCol.setUniformInt
                    (
                        "paletteData",
                        paletteDataUnit, 
                        false
                    );
                }
                computeShader_setNextPaletteCol.run(1, 1, 1);
            }
        }

        // Set uniforms
        computeShader_buildClustersFromPalette.setUniformInt
        (
            "image", 
            inputUnit
        );
        computeShader_buildClustersFromPalette.setUniformInt
        (
            "paletteData", 
            paletteDataUnit, 
            false
        );
        computeShader_updatePaletteFromClusters.setUniformInt
        (
            "paletteData", 
            paletteDataUnit
        );
        computeShader_updatePaletteFromClusters.setUniformInt
        (
            "image", 
            inputUnit, 
            false
        );
        computeShader_updatePaletteFromClusters.setUniformInt
        (
            "imageWidth", 
            mWidth, 
            false
        );
        computeShader_updatePaletteFromClusters.setUniformInt
        (
            "imageHeight", 
            mHeight, 
            false
        );

        // Run K-Means
        float xy(mWidth*mHeight);
        float sqErr0 = 3.0f*255.0f*255.0f*xy;
        int iter = 0;
        auto sqErrPtr = new uint32_t;
        relTol = std::min(std::max(relTol, 0.0f), 1.0f);
        while(true)
        {
            // Cluster colors around current palette colors
            computeShader_buildClustersFromPalette.use();
            computeShader_buildClustersFromPalette.run(mWidth, mHeight, 1);

            // Read clustering error
            glGetBufferSubData
            (
                GL_ATOMIC_COUNTER_BUFFER, 
                0, 
                sizeof(uint32_t), 
                sqErrPtr
            );

            // Quantize with currently available palette on break
            if (*sqErrPtr >= (1.0f-relTol)*sqErr0)
                break;
            sqErr0 = *sqErrPtr;
            
            // Update palettes based on current clusters
            computeShader_updatePaletteFromClusters.use();
            computeShader_updatePaletteFromClusters.run(paletteSize, 1, 1);
            iter++;
        }
        delete sqErrPtr;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    }

    // Bind quantizer inputs
    computeShader_quantizeInput.setUniformInt
    (
        "image", 
        inputUnit
    );
    computeShader_quantizeInput.setUniformInt
    (
        "paletteData", 
        paletteDataUnit, 
        false
    );
    computeShader_quantizeInput.setUniformInt
    (
        "paletteSize", 
        paletteSize, 
        false
    );
    computeShader_quantizeInput.setUniformInt
    (
        "indexedImage", 
        indexedDataUnit, 
        false
    );
    computeShader_quantizeInput.setUniformInt
    (
        "ditherLevel", 
        ditherLevel, 
        false
    );
    computeShader_quantizeInput.setUniformInt
    (
        "alphaCutoff", 
        alphaCutoff, 
        false
    );
    computeShader_quantizeInput.setUniformInt
    (
        "computeDelta", 
        int(computeDelta), 
        false
    );
    if (ditherLevel > 0)
    {
        if (ditherThreshold == 0)
            ditherThreshold = 1.0f/std::sqrt(float(paletteSize));
        else 
            ditherThreshold = std::min(std::max(ditherThreshold,0.0f),1.0f);
        computeShader_quantizeInput.setUniformFloat
        (
            "ditherThreshold", 
            ditherThreshold, 
            false
        );
    } 
    if (computeDelta)
    {
        computeShader_quantizeInput.setUniformInt
        (
            "oldQuantizedImage", 
            oldQuantizedInputUnit, 
            false
        );
    }

    // Quantize input with k-means-determined palettes
    glActiveTexture(GL_TEXTURE0+inputUnit);
    glBindTexture(GL_TEXTURE_2D, id);
    glBindImageTexture(inputUnit, id, 0, GL_FALSE, 0, GL_READ_WRITE,GL_RGBA8UI);
    computeShader_quantizeInput.use();
    computeShader_quantizeInput.run(width, height, 1);

    // Copy the palette to the PBO (GPU-to-GPU) so that it can be
    // read via the permanent mapping at mappedPaletteData_
    glBindBuffer(GL_PIXEL_PACK_BUFFER, paletteDataPBO_);
    glActiveTexture(GL_TEXTURE0+paletteDataUnit);
    glBindTexture(GL_TEXTURE_2D, paletteData_);
    glGetTexImage
    (
        GL_TEXTURE_2D, 
        0, 
        GL_RED_INTEGER,
        GL_UNSIGNED_BYTE, 
        0
    );
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    
    // Copy the indexed texture to the PBO (GPU-to-GPU) so that it can be
    // read via the permanent mapping at mappedIndexedData_
    glBindBuffer(GL_PIXEL_PACK_BUFFER, indexedDataPBO_);
    glActiveTexture(GL_TEXTURE0+indexedDataUnit);
    glBindTexture(GL_TEXTURE_2D, indexedData_);
    glGetTexImage
    (
        GL_TEXTURE_2D, 
        0, 
        GL_RED_INTEGER, 
        GL_UNSIGNED_BYTE, 
        0
    );
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    waitSync();
    resetSync();    // For whatever reason, this appears to be vital to ensure
                    // proper overwriting of mappedPaletteData
    
    if (regenerateMipmap)
    {
        glActiveTexture(GL_TEXTURE0+inputUnit);
        glBindTexture(GL_TEXTURE_2D, id);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void OpenGLKMeansQuantizer::waitSync()
{
    if (firstWaitSyncCall_)
    {   
        dataSync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        firstWaitSyncCall_ = false;
    }
    while (dataSync_)
    {
        GLenum wait = glClientWaitSync(dataSync_, 
            GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED)
            break;
    }
}

void OpenGLKMeansQuantizer::resetSync()
{
    if (dataSync_)
		glDeleteSync(dataSync_);
	dataSync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

//----------------------------------------------------------------------------//
// Constructor, destructor

OpenGLKMeansQuantizer::OpenGLKMeansQuantizer() :
firstWaitSyncCall_(true)
{
    // Compile compute shader stages
    if (!OpenGLKMeansQuantizer::computeShaderStagesCompiled)
    {
        computeShader_findMaxSqrDistCol.compile();
        computeShader_setNextPaletteCol.compile();
        computeShader_buildClustersFromPalette.compile();
        computeShader_updatePaletteFromClusters.compile();
        computeShader_quantizeInput.compile();
        OpenGLKMeansQuantizer::computeShaderStagesCompiled = true;
    }

    // Generate data texture (will be resized to the correct size during 
    // quantization)
    glGenTextures(1, &paletteData_);
    glBindTexture(GL_TEXTURE_2D, paletteData_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D
    (
        GL_TEXTURE_2D, 
        0, 
        GL_R32UI, 
        1, 
        3, 
        0, 
        GL_RED_INTEGER, 
        GL_UNSIGNED_INT, 
        NULL
    );

    // Generate pixel buffer object (PBO) for reading palette data from GPU
    // to CPU
    glGenBuffers(1, &paletteDataPBO_);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, paletteDataPBO_);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    //
    glGenBuffers(1, &paletteDataWriteOnlyPBO_);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, paletteDataWriteOnlyPBO_);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Generete indexed input texture (will be resized to correct size during
    // quantization)
    glGenTextures(1, &indexedData_);
    glBindTexture(GL_TEXTURE_2D, indexedData_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D
    (
        GL_TEXTURE_2D, 
        0, 
        GL_R8UI, 
        1, 
        1, 
        0, 
        GL_RED_INTEGER, 
        GL_UNSIGNED_INT, 
        NULL
    );

    // Generate pixel buffer object (PBO) for reading indexed texture from GPU
    // to CPU
    glGenBuffers(1, &indexedDataPBO_);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, indexedDataPBO_);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Old quantized input storage texture generation
    glGenTextures(1, &oldQuantizedInput_);
    glBindTexture(GL_TEXTURE_2D, oldQuantizedInput_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D
    (
        GL_TEXTURE_2D, 
        0, 
        GL_RGBA, 
        1, 
        1, 
        0, 
        GL_RGBA, 
        GL_UNSIGNED_BYTE, 
        NULL
    );

    // Generate atomic counter for storing clustering error
    glGenBuffers(1, &clusteringError_);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, clusteringError_);
    uint32_t data(0);
    glBufferData
    (
        GL_ATOMIC_COUNTER_BUFFER, 
        sizeof(uint32_t), 
        &data, 
        GL_STATIC_READ
    );
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

OpenGLKMeansQuantizer::~OpenGLKMeansQuantizer()
{
    glDeleteTextures(1, &paletteData_);
    glDeleteBuffers(1, &paletteDataPBO_);
    glDeleteTextures(1, &indexedData_);
    glDeleteBuffers(1, &indexedDataPBO_);
    glDeleteBuffers(1, &clusteringError_);
    glDeleteTextures(1, &oldQuantizedInput_);
}

// 
void OpenGLKMeansQuantizer::quantize
(
    TextureBuffer2D* input, 
    uint32_t paletteSize, 
    unsigned char* palette,
    uint32_t ditherLevel,
    bool reseedPalette,
    bool recalculatePalette, 
    float relTol,
    float ditherThreshold,
    int alphaCutoff,
    bool regenerateMipmap,
    bool fastKMeans,
    bool computeDelta,
    uint32_t inputUnit
)
{
    if (input == nullptr)
        return;
    quantizeOpenGLTexture
    (
        input->id(),
        input->width(),
        input->height(),
        paletteSize,
        palette,
        ditherLevel,
        reseedPalette,
        recalculatePalette, 
        relTol,
        ditherThreshold,
        alphaCutoff,
        regenerateMipmap,
        fastKMeans,
        computeDelta,
        inputUnit
    );
}

// 
void OpenGLKMeansQuantizer::quantize
(
    Framebuffer* input, 
    uint32_t paletteSize, 
    unsigned char* palette,
    uint32_t ditherLevel,
    bool reseedPalette,
    bool recalculatePalette, 
    float relTol,
    float ditherThreshold,
    int alphaCutoff,
    bool regenerateMipmap,
    bool fastKMeans,
    bool computeDelta,
    uint32_t inputUnit
)
{
    if (input == nullptr)
        return;
    quantizeOpenGLTexture
    (
        input->colorBufferId(),
        input->width(),
        input->height(),
        paletteSize,
        palette,
        ditherLevel,
        reseedPalette,
        recalculatePalette, 
        relTol,
        ditherThreshold,
        alphaCutoff,
        regenerateMipmap,
        fastKMeans,
        computeDelta,
        inputUnit
    );
}

void OpenGLKMeansQuantizer::getPalette(unsigned char*& data, bool allocate)
{
    int paletteSize = deltaComputed_ ? paletteSize_ + 1 : paletteSize_;
    if (allocate)
        data = new unsigned char[3*paletteSize];
    waitSync();
    std::memcpy
    (
        data, 
        (unsigned char*)mappedPaletteData_, 
        paletteSize_*3*sizeof(unsigned char)
    );
    if (deltaComputed_)
    {
        data[3*paletteSize-3] = (unsigned char)0;
        data[3*paletteSize-2] = (unsigned char)0;
        data[3*paletteSize-1] = (unsigned char)0;
    }
    resetSync();
}

void OpenGLKMeansQuantizer::getIndexedTexture
(
    unsigned char*& data, 
    bool allocate
)
{
    int nPixels(width_*height_);
    if (allocate)
        data = new unsigned char[nPixels];
    waitSync();
    std::memcpy
    (
        data, 
        (unsigned char*)mappedIndexedData_, 
        nPixels*sizeof(unsigned char)
    );
    resetSync();
}

}