#pragma once

#include <string>
#include <vector>
#include "vir/include/vir.h"
#include "thirdparty/glm/glm.hpp"

namespace vir
{
    class UniformBuffer;
    class Camera;
}

namespace ShaderThing
{

class SharedUniforms : vir::Event::Receiver
{
private:

    struct Flags
    {
        const bool updateDataRangeI = true;
        bool updateDataRangeII = false;
        bool updateDataRangeIII = false;
        bool isTimeResetOnRenderingRestart = true;
    };

    // A properly aligned layout-std140 compilant C++ representation of the
    // ShaderThing shared uniform block. Grouped by update frequency for 
    // convenience of passing only certain ranges of it to the GPU UniformBlock
    // at a time
    struct Block
    {
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
                                                             // Range (bytes)
        // High update frequency (data range I)              // 
                     float      iTime        = 0.f;          // 0  -  4
                     int        iFrame       = 0;            // 4  -  8
                     int        iRenderPass  = 0;            // 8  - 12
        // Medium update frequency (data range II)
                     int        iUserAction  = false;        // 12 - 16
                     vec3A16    iWASD        = {0,0,-1};     // 16 - 32
                     vec3A16    iLook        = {0,0,1};      // 32 - 64
                     glm::vec4  iMouse       = {0,0,0,0};    // 64 - 80
        // Low update frequency (data range III)
                     glm::mat4  iMVP         = glm::mat4(0); // 80 -144
        alignas(8)   float      iAspectRatio = 1.f;          // 144-152
                     glm::ivec2 iResolution  = {512,512};    // 152-160

        // Whole array never updated all at once, only one array element at a
        // time, medium update frequency 
                     ivec3A16   iKeyboard[256] {};           // 160-4256

        static const uint32_t dataRangeICumulativeSize()   {return 12;}
        
        static const uint32_t dataRangeIICumulativeSize()  {return 80;}
        
        static const uint32_t dataRangeIIICumulativeSize() {return 160;}
        
        static const uint32_t iKeyboardKeyOffset(int iKey) {return 160+iKey*16;}
        static const uint32_t iKeyboardKeySize()           {return 16;}
        static const uint32_t size()                       {return 4256;}

        
        static constexpr const char* glslName = "SharedBlock";
        // The order of the uniforms within the block source must be the same as the
        // order in which they have been delcared in CPUBlock. On the other hand,
        // the actual uniform names do not matter, but I keep them mapped for
        // consistency
        static constexpr const char* glslSource =
R"(layout(std140) uniform SharedBlock {
    float iTime;
    int   iFrame;
    int   iRenderPass;
    int   iUserAction;
    vec3  iWASD;
    vec3  iLook;
    vec4  iMouse;
    mat4  iMVP;
    float iWindowAspectRatio;
    ivec2 iWindowResolution;
    ivec3 iKeyboard[256];};
)";
    };
    
    const unsigned int        gpuBindingPoint_;
          Flags               flags_    = {};
          Block               cpuBlock_ = {};
          vir::UniformBuffer* gpuBlock_ = nullptr;
        
          // Fixed camera used to retrieve the value of the projection view 
          // matrix iMVP
          vir::Camera*        screenCamera_   = nullptr;

          // Movable camera which responds to keyboard and mouse controls and is
          // used to provide values to cpuBlock.iWASD, cpuBlock.iLook
          vir::Camera*   shaderCamera_   = nullptr;

    void setResolution(glm::ivec2& resolution, bool windowFrameManuallyDragged);

public:

    SharedUniforms(const unsigned int bindingPoint = 0);
    ~SharedUniforms();

    DECLARE_RECEIVABLE_EVENTS
    (
        vir::Event::Type::WindowResize *
        vir::Event::Type::MouseButtonPress *
        vir::Event::Type::MouseMotion *
        vir::Event::Type::MouseButtonRelease *
        vir::Event::Type::KeyPress *
        vir::Event::Type::KeyRelease
    )
    void onReceive(vir::Event::WindowResizeEvent& e) override;
    void onReceive(vir::Event::MouseButtonPressEvent& e) override;
    void onReceive(vir::Event::MouseMotionEvent& e) override;
    void onReceive(vir::Event::MouseButtonReleaseEvent& e) override;
    void onReceive(vir::Event::KeyPressEvent& e) override;
    void onReceive(vir::Event::KeyReleaseEvent& e) override;

    const char* glslBlockSource() const {return cpuBlock_.glslSource;}

    void bindShader(vir::Shader* shader) const;

    void update();
    
    void renderWindowResolutionMenuGUI();
    void renderRowsGUI();
};

}