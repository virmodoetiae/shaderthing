#include "vpch.h"
#include <cmath>
#include "vgraphics/vpostprocess/vopengl/vopenglquantizer.h"
#include "vgraphics/vcore/vopengl/vopenglmisc.h"

namespace vir
{

// OpenGLKMeanQuantizer static data ------------------------------------------//

bool OpenGLQuantizer::computeShaderStagesCompiled = false;

OpenGLComputeShader 
    OpenGLQuantizer::computeShader_findMaxSqrDistColSF32
    (
R"(#version 460 core
uniform int counter;
uniform int paletteSize;
layout(binding=0) uniform atomic_uint clusteringError;
layout(rgba32f, binding=0) uniform image2D image;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uvec4 to8ui(vec4 v)
{
    return uvec4(255.0*min(max(v, 0), 1)+.5);
}
void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 d2MaxPos = ivec2(paletteSize+1,2);
    if (counter == 0)
    {
        if (gid == ivec2(0,0))
        {
            uvec4 img = to8ui(imageLoad(image, gid));
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
    ivec3 img = ivec3(to8ui(imageLoad(image, gid)).rgb);
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

OpenGLComputeShader
    OpenGLQuantizer::computeShader_setNextPaletteColSF32
    (
R"(#version 460 core
uniform int counter;
uniform int paletteSize;
layout(rgba32f, binding=0) uniform image2D image;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uvec4 to8ui(vec4 v)
{
    return uvec4(255*min(max(v, 0), 1)+.5);
}
void main()
{
    if (counter == 0)
        return;
    uint x = imageLoad(paletteData, ivec2(paletteSize+2,2)).r;
    uint y = imageLoad(paletteData, ivec2(paletteSize+3,2)).r;
    uvec4 img = to8ui(imageLoad(image, ivec2(x,y)));
    imageStore(paletteData, ivec2(3*counter,0), uvec4(img.r,0,0,1));
    imageStore(paletteData, ivec2(3*counter+1,0), uvec4(img.g,0,0,1));
    imageStore(paletteData, ivec2(3*counter+2,0), uvec4(img.b,0,0,1));
    ivec2 d2MaxPos = ivec2(paletteSize+1,2);
    imageStore(paletteData, d2MaxPos, uvec4(0,0,0,0));
})" );

OpenGLComputeShader
    OpenGLQuantizer::computeShader_buildClustersFromPaletteSF32
    (
R"(#version 460 core
uniform int paletteSize;
layout(binding=0) uniform atomic_uint clusteringError;
layout(rgba32f, binding=1) uniform image2D image;
layout(r32ui, binding=2) uniform uimage2D paletteData;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uvec4 to8ui(vec4 v)
{
    return uvec4(255*min(max(v, 0), 1)+.5);
}
void main() 
{ 
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    uvec4 img = to8ui(imageLoad(image, gid));
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

OpenGLComputeShader
    OpenGLQuantizer::computeShader_updatePaletteFromClustersSF32
    (
R"(#version 460 core
layout(binding=0) uniform atomic_uint clusteringError;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(rgba32f, binding=2) uniform image2D image;
layout(rgba8ui, binding=3) uniform uimage2D cumulatedPaletteData;
uniform int imageWidth;
uniform int imageHeight;
uniform int cumulatedPaletteRow;
uniform int cumulatePalette;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
float rand(vec2 seed)
{
    return fract(sin(dot(seed.xy, vec2(12.9898, 78.233)))*43758.5453);
}
uvec4 to8ui(vec4 v)
{
    return uvec4(255*min(max(v, 0), 1)+.5);
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
        uvec4 col = to8ui(imageLoad(image, ivec2(x,y)));
        r = col.r;
        g = col.g;
        b = col.b;
    }
    uvec3 color = uvec3(vec3(r,g,b)/count);
    imageAtomicExchange(paletteData, ivec2(gid*3,   0), color.r);
    imageAtomicExchange(paletteData, ivec2(gid*3+1, 0), color.g);
    imageAtomicExchange(paletteData, ivec2(gid*3+2, 0), color.b);
    if (cumulatePalette == 1)
        imageStore(cumulatedPaletteData,ivec2(gid,cumulatedPaletteRow),uvec4(color,255));
})" );

OpenGLComputeShader
    OpenGLQuantizer::computeShader_quantizeInputSF32
    (
R"(#version 460 core
uniform int paletteSize;
uniform int ditherLevel;
uniform float ditherThreshold;
uniform int alphaCutoff;
uniform int indexMode;
layout(rgba32f, binding=0) uniform image2D inputImage;
layout(rgba32f, binding=4) uniform image2D outputImage;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(r8ui, binding=2) uniform uimage2D indexedImage;
layout(rgba32f, binding=3) uniform image2D oldQuantizedImage;
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
uvec4 to8ui(vec4 v)
{
    return uvec4(255*min(max(v, 0), 1)+.5);
}
vec4 to32f(uvec4 v)
{
    return vec4(v)/255.0;
}
void main() 
{ 
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    uvec4 img = to8ui(imageLoad(inputImage, gid));
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
    if (indexMode == 1 && a == 0)
        index = paletteSize;
    else if (indexMode == 2)
    {
        uvec4 oldColor = to8ui(imageLoad(oldQuantizedImage, gid));
        if (oldColor.a > 0 && newColor.rgb == oldColor.rgb)
        {
            index = paletteSize;
        }
        imageStore(oldQuantizedImage, gid, to32f(newColor));
    }
    imageStore(outputImage, gid, to32f(newColor));
    imageStore(indexedImage, gid, uvec4(index, 0, 0, 0));
})" );

// Same compute shaders as above, but to operate on unsigned int and unsigned
// normlaized int -type of textures

OpenGLComputeShader
    OpenGLQuantizer::computeShader_findMaxSqrDistColUI8
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

OpenGLComputeShader
    OpenGLQuantizer::computeShader_setNextPaletteColUI8
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

OpenGLComputeShader
    OpenGLQuantizer::computeShader_buildClustersFromPaletteUI8
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

OpenGLComputeShader
    OpenGLQuantizer::computeShader_updatePaletteFromClustersUI8
    (
R"(#version 460 core
layout(binding=0) uniform atomic_uint clusteringError;
layout(r32ui, binding=1) uniform uimage2D paletteData;
layout(rgba8ui, binding=2) uniform uimage2D image;
layout(rgba8ui, binding=3) uniform uimage2D cumulatedPaletteData;
uniform int imageWidth;
uniform int imageHeight;
uniform int cumulatedPaletteRow;
uniform int cumulatePalette;
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
    uvec3 color = uvec3(vec3(r,g,b)/count);
    imageAtomicExchange(paletteData, ivec2(gid*3,   0), color.r);
    imageAtomicExchange(paletteData, ivec2(gid*3+1, 0), color.g);
    imageAtomicExchange(paletteData, ivec2(gid*3+2, 0), color.b);
    if (cumulatePalette == 1)
        imageStore(cumulatedPaletteData,ivec2(gid,cumulatedPaletteRow),uvec4(color,255));
})" );

OpenGLComputeShader
    OpenGLQuantizer::computeShader_quantizeInputUI8
    (
R"(#version 460 core
uniform int paletteSize;
uniform int ditherLevel;
uniform float ditherThreshold;
uniform int alphaCutoff;
uniform int indexMode;
layout(rgba8ui, binding=0) uniform uimage2D inputImage;
layout(rgba8ui, binding=4) uniform uimage2D outputImage;
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
    uvec4 img = uvec4(imageLoad(inputImage, gid));
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
    if (indexMode == 1 && a == 0)
        index = paletteSize;
    else if (indexMode == 2)
    {
        uvec4 oldColor = imageLoad(oldQuantizedImage, gid);
        if (oldColor.a > 0 && newColor.rgb == oldColor.rgb)
        {
            index = paletteSize;
        }
        imageStore(oldQuantizedImage, gid, newColor);
    }
    imageStore(outputImage, gid, newColor);
    imageStore(indexedImage, gid, uvec4(index, 0, 0, 0));
})" );


//----------------------------------------------------------------------------//
// Private member functions

void OpenGLQuantizer::quantizeOpenGLTexture
(
    GLuint id,
    uint32_t width,
    uint32_t height,
    unsigned int paletteSize,
    const Settings& settings,
    bool isFloat32
)
{
    //
    uint32_t textureUnit = settings.inputUnit;
    uint32_t inputDataUnit = textureUnit++;

    // Figure out which compute shaders to use based on input texture
    // internal format
    OpenGLComputeShader* findMaxSqrDistCol;
    OpenGLComputeShader* setNextPaletteCol;
    OpenGLComputeShader* buildClustersFromPalette;
    OpenGLComputeShader* updatePaletteFromClusters;
    OpenGLComputeShader* quantizeInput;
    if(isFloat32)
    {
        findMaxSqrDistCol = &computeShader_findMaxSqrDistColSF32;
        setNextPaletteCol = &computeShader_setNextPaletteColSF32;
        buildClustersFromPalette = 
            &computeShader_buildClustersFromPaletteSF32;
        updatePaletteFromClusters = 
            &computeShader_updatePaletteFromClustersSF32;
        quantizeInput = &computeShader_quantizeInputSF32;
    }
    else
    {
        findMaxSqrDistCol = &computeShader_findMaxSqrDistColUI8;
        setNextPaletteCol = &computeShader_setNextPaletteColUI8;
        buildClustersFromPalette = 
            &computeShader_buildClustersFromPaletteUI8;
        updatePaletteFromClusters = 
            &computeShader_updatePaletteFromClustersUI8;
        quantizeInput = &computeShader_quantizeInputUI8;
    }

    // Cache for possibly re-initializing buffers
    bool paletteSizeChanged(paletteSize_ != paletteSize);
    bool internalFormatChanged(isFloat320_ != isFloat32);
    
    // Cache settings
    settings_ = settings;

    // Limit and adjust palette size (no reason why more than 256 colors can't
    // be processed, however, I feel an upper bound can't hurt, also because
    // it's hard to tell a difference above 256 colors, unless very smooth color
    // gradients are at play). Also, I want this to play nice with the GIF 
    // encoder, which does have a hard limit at 256 colors, so...
    paletteSize = 
        std::min
        (
            std::max(paletteSize, 2u), 
            settings_.indexMode != Settings::IndexMode::Default ? 255u : 256u
        );
    
    //
    if 
    (
        settings_.indexMode == Settings::IndexMode::Alpha && 
        settings_.alphaCutoff == -1
    )
    {
        settings_.alphaCutoff = 127;
    }
    
    // Bind buffers
    // Input image
    glActiveTexture(GL_TEXTURE0+inputDataUnit);
    glBindTexture(GL_TEXTURE_2D, id);

    uint32_t targetLevel = 0;
    int mWidth = width;
    int mHeight = height;
    if 
    (
        settings_.fastKMeans && 
        (
            paletteSizeChanged ||
            settings_.recalculatePalette ||
            internalFormatChanged
        ) && width*height > 2500
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
        settings_.inputUnit, 
        id, 
        targetLevel, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        isFloat32 ? GL_RGBA32F : GL_RGBA8UI
    );

    // Palette data texture
    uint32_t paletteDataUnit = textureUnit++;
    glActiveTexture(GL_TEXTURE0+paletteDataUnit);
    glBindTexture(GL_TEXTURE_2D, paletteData_);
    glBindImageTexture(paletteDataUnit, paletteData_, 0, GL_FALSE, 0, 
        GL_READ_WRITE, GL_R32UI);
    // Cumulated palette data
    uint32_t cumulatedPaletteDataUnit = textureUnit++;
    glActiveTexture(GL_TEXTURE0+cumulatedPaletteDataUnit);
    glBindTexture(GL_TEXTURE_2D, cumulatedPaletteData_);
    glBindImageTexture
    (
        cumulatedPaletteDataUnit, 
        cumulatedPaletteData_, 
        0, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        GL_RGBA8UI
    );
    // Indexed data texture
    uint32_t indexedDataUnit = textureUnit++;
    glActiveTexture(GL_TEXTURE0+indexedDataUnit);
    glBindTexture(GL_TEXTURE_2D, indexedData_);
    glBindImageTexture(indexedDataUnit, indexedData_, 0, GL_FALSE, 0, 
        GL_READ_WRITE, GL_R8UI);
    //
    uint32_t oldQuantizedInputUnit = textureUnit++;
    // Atomic error counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, clusteringError_);
    uint32_t binding = 0;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, 
        clusteringError_);

    // Resize indexedData if necessary
    if (width_ != width || height_ != height || internalFormatChanged)
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
        if (settings_.indexMode == Settings::IndexMode::Delta)
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
                isFloat32 ? GL_RGBA32F : GL_RGBA8,
                width, 
                height, 
                0, 
                GL_RGBA, 
                isFloat32 ? GL_FLOAT : GL_UNSIGNED_BYTE, 
                NULL
            );
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    if (settings_.indexMode == Settings::IndexMode::Delta)
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
            isFloat32 ? GL_RGBA32F : GL_RGBA8UI // 8UI
        );
    }

    // Resize paletteData if necessary
    if (paletteSizeChanged || internalFormatChanged)
    {
        settings_.reseedPalette = true;
        settings_.recalculatePalette = true;
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
            3*paletteSize*sizeof(uint32_t), 
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
            3*paletteSize*sizeof(uint32_t), 
            WriteOnlyPBOFlags
        );
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        
        // Reset uniforms
        findMaxSqrDistCol->setUniformInt
        (
            "paletteSize", 
            paletteSize
        );
        setNextPaletteCol->setUniformInt
        (
            "paletteSize", 
            paletteSize
        );
        buildClustersFromPalette->setUniformInt
        (
            "paletteSize", 
            paletteSize
        );
    }

    if (paletteSizeChanged)
    {
        glActiveTexture(GL_TEXTURE0+cumulatedPaletteDataUnit);
        glBindTexture(GL_TEXTURE_2D, cumulatedPaletteData_);
        glTexImage2D
        (
            GL_TEXTURE_2D, 
            0, 
            GL_RGBA8, 
            paletteSize, 
            maxNCumulatedPalettes_, 
            0, 
            GL_RGBA, 
            GL_UNSIGNED_BYTE, 
            NULL
        );
    }

    // If provided, upload the palette to the GPU and use it instead of running
    // the KMeans
    if (settings_.paletteData != nullptr)
    {
        settings_.recalculatePalette = false;
        glActiveTexture(GL_TEXTURE0+paletteDataUnit);
        glBindTexture(GL_TEXTURE_2D, paletteData_);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, paletteDataWriteOnlyPBO_);
        OpenGLWaitSync();
        //waitSync();
        std::memcpy
        (
            mappedWriteOnlyPaletteData_, 
            (void*)settings_.paletteData, 
            3*paletteSize*sizeof(unsigned char)
        );
        //resetSync();
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

    if (settings_.recalculatePalette || internalFormatChanged)
    {
        // Build initial palette

        // I suspect that that the largest bottleneck is this first CPU-based 
        // loop from which async OpenGL commands are issued. Is there a 
        // smarter way?
        if (settings_.reseedPalette || internalFormatChanged)
        {
            for (int counter = 0; counter < paletteSize; counter++)
            {
                // Determine the pixel of the image which is the furthest away
                // (in color-space) from the last palette color in paletteData
                findMaxSqrDistCol->setUniformInt
                (
                    "counter", 
                    counter
                );
                if (counter == 0)
                {
                    findMaxSqrDistCol->setUniformInt
                    (
                        "image", 
                        settings_.inputUnit, 
                        false
                    );
                    findMaxSqrDistCol->setUniformInt
                    (
                        "paletteData",
                        paletteDataUnit,
                        false
                    );
                }
                findMaxSqrDistCol->run(mWidth, mHeight, 1);

                // Add the previously selected image pixel to the palette set,
                // reset max distance
                setNextPaletteCol->setUniformInt
                (
                    "counter", 
                    counter
                );
                if (counter == 0)
                {
                    setNextPaletteCol->setUniformInt
                    (
                        "image", 
                        settings_.inputUnit, 
                        false
                    );
                    setNextPaletteCol->setUniformInt
                    (
                        "paletteData",
                        paletteDataUnit, 
                        false
                    );
                }
                setNextPaletteCol->run(1, 1, 1);
            }
        }

        // Set uniforms
        buildClustersFromPalette->setUniformInt
        (
            "image", 
            settings_.inputUnit
        );
        buildClustersFromPalette->setUniformInt
        (
            "paletteData", 
            paletteDataUnit, 
            false
        );
        updatePaletteFromClusters->setUniformInt
        (
            "paletteData", 
            paletteDataUnit
        );
        updatePaletteFromClusters->setUniformInt
        (
            "image", 
            settings_.inputUnit, 
            false
        );
        updatePaletteFromClusters->setUniformInt
        (
            "imageWidth", 
            mWidth, 
            false
        );
        updatePaletteFromClusters->setUniformInt
        (
            "imageHeight", 
            mHeight, 
            false
        );
        if (settings_.cumulatePalette)
        {
            updatePaletteFromClusters->setUniformInt
            (
                "cumulatePalette", 
                1,
                false
            );
            updatePaletteFromClusters->setUniformInt
            (
                "cumulatedPaletteData", 
                cumulatedPaletteDataUnit,
                false
            );
            updatePaletteFromClusters->setUniformInt
            (
                "cumulatedPaletteRow", 
                cumulatedPaletteRow_,
                false
            );
        }
        else
            updatePaletteFromClusters->setUniformInt
            (
                "cumulatePalette", 
                0,
                false
            );

        // Run K-Means
        float sqErr0 = 3.0f*255.0f*255.0f*mWidth*mHeight;
        auto sqErrPtr = new uint32_t;
        settings_.relTol = std::min(std::max(settings_.relTol, 0.0f), 1.0f);
        while(true)
        {
            // Cluster colors around current palette colors
            buildClustersFromPalette->use();
            buildClustersFromPalette->run(mWidth, mHeight, 1);

            // Read clustering error
            glGetBufferSubData
            (
                GL_ATOMIC_COUNTER_BUFFER, 
                0, 
                sizeof(uint32_t), 
                sqErrPtr
            );

            // Quantize with currently available palette on break
            if (*sqErrPtr >= (1.0f-settings_.relTol)*sqErr0)
                break;
            sqErr0 = *sqErrPtr;
            
            // Update palettes based on current clusters
            updatePaletteFromClusters->use();
            updatePaletteFromClusters->run(paletteSize, 1, 1);
        }
        delete sqErrPtr;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        
        cumulatedPaletteRow_ = 
            settings_.cumulatePalette ? 
            (
                ++cumulatedPaletteRow_ < maxNCumulatedPalettes_ ? 
                cumulatedPaletteRow_ : 0u 
            ) : 0u;
    }

    // Bind quantizer inputs
    quantizeInput->setUniformInt
    (
        "inputImage", 
        settings_.inputUnit
    );
    quantizeInput->setUniformInt
    (
        "paletteData", 
        paletteDataUnit, 
        false
    );
    quantizeInput->setUniformInt
    (
        "paletteSize", 
        paletteSize, 
        false
    );
    quantizeInput->setUniformInt
    (
        "indexedImage", 
        indexedDataUnit, 
        false
    );
    quantizeInput->setUniformInt
    (
        "ditherLevel", 
        (int)settings_.ditherMode, 
        false
    );
    quantizeInput->setUniformInt
    (
        "alphaCutoff", 
        settings_.alphaCutoff, 
        false
    );
    quantizeInput->setUniformInt
    (
        "indexMode", 
        int(settings_.indexMode), 
        false
    );
    if (settings_.ditherMode != Settings::DitherMode::None)
    {
        if (settings_.ditherThreshold == 0)
            settings_.ditherThreshold = 1.0f/std::sqrt(float(paletteSize));
        else 
            settings_.ditherThreshold = 
                std::min(std::max(settings_.ditherThreshold,0.0f),1.0f);
        quantizeInput->setUniformFloat
        (
            "ditherThreshold", 
            settings_.ditherThreshold, 
            false
        );
    } 
    if (settings_.indexMode == Settings::IndexMode::Delta)
    {
        quantizeInput->setUniformInt
        (
            "oldQuantizedImage", 
            oldQuantizedInputUnit, 
            false
        );
    }

    // Quantize input with k-means-determined palettes
    glActiveTexture(GL_TEXTURE0+settings_.inputUnit);
    glBindTexture(GL_TEXTURE_2D, id);
    glBindImageTexture
    (
        settings_.inputUnit, 
        id, 
        0, 
        GL_FALSE, 
        0, 
        settings_.overwriteInput ? GL_READ_WRITE : GL_READ_ONLY, 
        isFloat32 ? GL_RGBA32F : GL_RGBA8UI
    );
    uint32_t outputDataUnit;
    if (!settings_.overwriteInput)
    {
        outputDataUnit = textureUnit++;
        glActiveTexture(GL_TEXTURE0+outputDataUnit);
        glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
        glBindImageTexture
        (
            outputDataUnit, 
            output_->colorBufferId(), 
            0, 
            GL_FALSE, 
            0, 
            GL_WRITE_ONLY, 
            isFloat32 ? GL_RGBA32F : GL_RGBA8UI
        );
    }
    quantizeInput->setUniformInt
    (
        "outputImage", 
        settings_.overwriteInput ? settings_.inputUnit : outputDataUnit
    );
    quantizeInput->use();
    quantizeInput->run(width, height, 1);

    OpenGLWaitSync();

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
    // read via the permanent mapping at mappedIndexedData_. It is vital that
    // the pixel packing alignment is set to 1 (set in the constructor) for this
    // to be performed correctly
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
    
    if (settings_.regenerateMipmap)
    {
        if (settings_.overwriteInput)
        {
            glActiveTexture(GL_TEXTURE0+settings_.inputUnit);
            glBindTexture(GL_TEXTURE_2D, id);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0+outputDataUnit);
            glBindTexture(GL_TEXTURE_2D, output_->colorBufferId());
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    isFloat320_ = isFloat32;
}

//----------------------------------------------------------------------------//
// Constructor, destructor

OpenGLQuantizer::OpenGLQuantizer() //:
//firstWaitSyncCall_(true)
{
    auto context = Window::instance()->context();
    if (context->versionMajor() < 4)
        canRunOnDeviceInUse_ = false;
    else if (context->versionMinor() < 3)
        canRunOnDeviceInUse_ = false;
    if (!canRunOnDeviceInUse_)
    {
        auto* context(Window::instance()->context());
        std::string glVersion
        ( 
            std::to_string(context->versionMajor())+"."+
            std::to_string(context->versionMinor())
        );
        std::string deviceName(Renderer::instance()->deviceName());
        errorMessage_ =
R"(This feature requires an OpenGL version >= 4.3, but your graphics card in 
use ()"+deviceName+R"() only supports OpenGL up to version )"+glVersion;
        return;
    }

    // Compile compute shader stages
    if (!OpenGLQuantizer::computeShaderStagesCompiled)
    {
        computeShader_findMaxSqrDistColSF32.compile();
        computeShader_setNextPaletteColSF32.compile();
        computeShader_buildClustersFromPaletteSF32.compile();
        computeShader_updatePaletteFromClustersSF32.compile();
        computeShader_quantizeInputSF32.compile();
        computeShader_findMaxSqrDistColUI8.compile();
        computeShader_setNextPaletteColUI8.compile();
        computeShader_buildClustersFromPaletteUI8.compile();
        computeShader_updatePaletteFromClustersUI8.compile();
        computeShader_quantizeInputUI8.compile();
        OpenGLQuantizer::computeShaderStagesCompiled = true;
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

    // Generate cumulated palettes texture
    glGenTextures(1, &cumulatedPaletteData_);
    glBindTexture(GL_TEXTURE_2D, cumulatedPaletteData_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D
    (
        GL_TEXTURE_2D, 
        0, 
        GL_RGBA8, 
        1, 
        1, 
        0, 
        GL_RGBA, 
        GL_UNSIGNED_BYTE, 
        NULL
    );
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxNCumulatedPalettes_);

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

    // Finally, set pixel alignment for packing (e.g., texture->PBO transfer 
    // operations) to 1 byte instead of the deafult 4. Without this, the
    // packing of the indexedDataPBO will include padding values which will
    // make it useless if the input textures have width and height that are
    // not a power of two. Took a while to figure this one out...
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

OpenGLQuantizer::~OpenGLQuantizer()
{
    if (!canRunOnDeviceInUse_)
        return;
    glDeleteTextures(1, &paletteData_);
    glDeleteBuffers(1, &paletteDataPBO_);
    glDeleteBuffers(1, &paletteDataWriteOnlyPBO_);
    glDeleteTextures(1, &cumulatedPaletteData_);
    glDeleteTextures(1, &indexedData_);
    glDeleteBuffers(1, &indexedDataPBO_);
    glDeleteBuffers(1, &clusteringError_);
    glDeleteTextures(1, &oldQuantizedInput_);
}

// 
void OpenGLQuantizer::quantize
(
    TextureBuffer2D* input, 
    unsigned int paletteSize,
    const Settings& settings
)
{
    if (!canRunOnDeviceInUse_)
        return;
    if (input == nullptr)
        return;
    if (!settings.overwriteInput)
        prepareOutput(input);
    quantizeOpenGLTexture
    (
        input->id(),
        input->width(),
        input->height(),
        paletteSize,
        settings,
        input->internalFormat() == TextureBuffer::InternalFormat::RGBA_SF_32
    );
}

// 
void OpenGLQuantizer::quantize
(
    Framebuffer* input, 
    unsigned int paletteSize,
    const Settings& settings
)
{
    if (!canRunOnDeviceInUse_)
        return;
    if (input == nullptr)
        return;
    if (!settings.overwriteInput)
        prepareOutput(input);
    quantizeOpenGLTexture
    (
        input->colorBufferId(),
        input->width(),
        input->height(),
        paletteSize,
        settings,
        input->colorBufferInternalFormat() == 
            TextureBuffer::InternalFormat::RGBA_SF_32
    );
}

void OpenGLQuantizer::getPalette
(
    unsigned char*& data, 
    bool allocate, 
    bool cumulated
)
{
    if (!canRunOnDeviceInUse_)
        return;
    // Requesting the cumulated palette amounts to:
    // 1) quantizing the cumulated palette image
    // 2) returning the resulting quantization palette, which can be interpreted
    //    as a global palette best representing all palettes that have been 
    //    stored in cumulatedPaletteData_ up to this point
    // If no palettes have been stored in the cumulated palette data (i.e., if
    // cumulatedPaletteRow_ == 0) the latest quantizer palette is returned.
    // Please also note that retrieving the quantization palette reset the
    // cumulatedPaletteRow_ to 0
    if (cumulated && cumulatedPaletteRow_ > 0)
    {
        Settings settings = {};
        settings.fastKMeans = false;
        settings.recalculatePalette = true;
        settings.regenerateMipmap = false;
        settings.reseedPalette = true;
        quantizeOpenGLTexture
        (
            cumulatedPaletteData_,
            paletteSize_,
            cumulatedPaletteRow_,
            paletteSize_,
            settings,
            false
        );
        getPalette(data, allocate, false);
        return;
    }
    bool nonDefaultIndexing(settings_.indexMode != Settings::IndexMode::Default);
    int paletteSize = nonDefaultIndexing ? paletteSize_ + 1 : paletteSize_;
    if (allocate)
        data = new unsigned char[3*paletteSize];
    OpenGLWaitSync();
    std::memcpy
    (
        data, 
        (unsigned char*)mappedPaletteData_, 
        paletteSize_*3*sizeof(unsigned char)
    );
    // Add a dummy color to correspond to the alpha/delta index
    if (nonDefaultIndexing)
    {
        
        data[3*paletteSize-3] = (unsigned char)0;
        data[3*paletteSize-2] = (unsigned char)0;
        data[3*paletteSize-1] = (unsigned char)0;
    }
}

void OpenGLQuantizer::getIndexedTexture
(
    unsigned char*& data, 
    bool allocate
)
{
    if (!canRunOnDeviceInUse_)
        return;
    int nPixels(width_*height_);
    if (allocate)
        data = new unsigned char[nPixels];
    OpenGLWaitSync();
    std::memcpy
    (
        data, 
        (unsigned char*)mappedIndexedData_, 
        nPixels*sizeof(unsigned char)
    );
}

}