#ifndef ST_SHARED_UNIFORM_BLOCK_H
#define ST_SHARED_UNIFORM_BLOCK_H

#include "thirdparty/glm/glm.hpp"
#include "vir/include/vgraphics/vcore/vshader.h"

namespace ShaderThing
{
// NOT USED
// A properly aligned layout-std140 compilant C++ representation of the
// ShaderThing shared uniform block. Grouped by update frequency for convenience
// of passing only certain ranges of it to the GPU UniformBlock at a time
struct SharedUniformBlock
{
    struct alignas(16) ivec2A16
    {
        int x = 0;
        int y = 0;
        ivec2A16&  operator= (const glm::ivec2& v)       {x=v.x;y=v.y;return *this;}
        bool       operator==(const ivec2A16&   v) const {return x==v.x&&y==v.y;}
        bool       operator!=(const ivec2A16&   v) const {return !operator==(v);}
        bool       operator==(const glm::ivec2& v) const {return x==v.x&&y==v.y;}
        bool       operator!=(const glm::ivec2& v) const {return !operator==(v);}
        glm::ivec2 packed()                        const {return{x,y};}
    };
    struct alignas(16) ivec3A16
    {
        int x = 0;
        int y = 0; 
        int z = 0;
        ivec3A16&  operator= (const glm::ivec3& v)       {x=v.x;y=v.y;z=v.z;return *this;}
        bool       operator==(const ivec3A16&   v) const {return x==v.x&&y==v.y&&z==v.z;}
        bool       operator!=(const ivec3A16&   v) const {return !operator==(v);}
        bool       operator==(const glm::ivec3& v) const {return x==v.x&&y==v.y&&z==v.z;}
        bool       operator!=(const glm::ivec3& v) const {return !operator==(v);}
        glm::ivec3 packed()                        const {return{x,y,z};}
    };
    struct alignas(16) vec3A16
    {
        float x = 0; 
        float y = 0; 
        float z = 0;
        vec3A16&  operator= (const glm::vec3& v)       {x=v.x;y=v.y;z=v.z;return *this;}
        bool      operator==(const vec3A16&   v) const {return x==v.x&&y==v.y&&z==v.z;}
        bool      operator!=(const vec3A16&   v) const {return !operator==(v);}
        bool      operator==(const glm::vec3& v) const {return x==v.x&&y==v.y&&z==v.z;}
        bool      operator!=(const glm::vec3& v) const {return !operator==(v);}
        glm::vec3 packed()                       const {return{x,y,z};}
    };

    // High update frequency (data range I)
    alignas(16) float       iTime = 0.f;
    alignas(16) int         iFrame = 0;
    alignas(16) int         iRenderPass = 0;

    // Medium update frequency (data range II)
    alignas(16) bool        iUserAction = false;
                vec3A16     iWASD = {0,0, 1.f};
                vec3A16     iLook = {0,0,-1.f};
                glm::ivec4  iMouse = {0,0,0,0};

    // Low update frequency (data range III)
                glm::mat4   iMvp = glm::mat4(0);
    alignas(16) float       iAspectRatio = 1.f;
                ivec2A16    iResolution = {512,512};

    // Whole array never updated all at once, only one array element at a
    // time, medium update frequency 
                ivec3A16    iKeyboard[256] {};

    // Size/offset accessor for convenience
    const uint32_t dataRangeIOffset()           const {return 0;}
    const uint32_t dataRangeISize()             const {return 48;}
    const uint32_t dataRangeIIOffset()          const {return 48;}
    const uint32_t dataRangeIISize()            const {return 64;}
    const uint32_t dataRangeIIIOffset()         const {return 112;}
    const uint32_t dataRangeIIISize()           const {return 96;}
    const uint32_t iKeyboardKeyOffset(int iKey) const {return 208+iKey*16;}
    const uint32_t iKeyboardKeySize()           const {return 16;}
    const uint32_t iKeyboardSize()              const {return 4096;}
};

}

#endif