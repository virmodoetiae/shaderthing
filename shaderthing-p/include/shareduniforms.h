#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "shaderthing-p/include/uniform.h"
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

friend Uniform;

public :

    struct UpdateArgs
    {
        const float timeStep;
    };

private:

    struct Flags
    {
        const bool updateDataRangeI              = true;
              bool updateDataRangeII             = false;
              bool updateDataRangeIII            = false;
              bool restartRendering              = false;
              bool isRenderingPaused             = false;
              bool isTimePaused                  = false;
              bool isTimeLooped                  = false;
              bool isTimeResetOnRenderingRestart = true;
              bool isKeyboardInputEnabled        = true; // iKeyboard
              bool isMouseInputEnabled           = true; // iMouse
              bool isCameraKeyboardInputEnabled  = true; // iWASD
              bool isCameraMouseInputEnabled     = true; // iLook
    };

    // A properly aligned layout-std140 compilant C++ representation of the
    // ShaderThing shared uniform block. Grouped by update frequency for 
    // convenience of passing only certain ranges of it to the GPU UniformBlock
    // at a time. Somewhat wasteful because of the extensive usage of vec3s
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
                                                            // Range (bytes) | Size (bytes)
        // High update frequency (data range I)
                    float      iTime        = 0.f;          // 0  -  4       | 4
                    int        iFrame       = 0;            // 4  -  8       | 4
                    int        iRenderPass  = 0;            // 8  - 12       | 4
        // Medium update frequency (data range II)
                    bool       iUserAction  = false;        // 12 - 16       | 4
                    vec3A16    iWASD        = {0,0,-1};     // 16 - 32       | 16
                    vec3A16    iLook        = {0,0,1};      // 32 - 48       | 16
                    glm::vec4  iMouse       = {0,0,0,0};    // 48 - 64       | 16
        // Low update frequency (data range III)
                    glm::mat4  iMVP         = glm::mat4(0); // 64 -128       | 64
                    float      iAspectRatio = 1.f;          // 128-132       | 4
        // Using alignas(8) to force iResolution to start 
        // at byte 136 instead of 132
        alignas(8)  glm::vec2  iResolution  = {512,512};    // 136-144       | 8

        // Whole array never updated all at once, only one 
        // array element at a time, medium update frequency
                    ivec3A16   iKeyboard[256] {};           // 144-4240      | 16*256 = 4096

        static const uint32_t dataRangeICumulativeSize()   {return 12;}
        static const uint32_t dataRangeIICumulativeSize()  {return 64;}
        static const uint32_t dataRangeIIICumulativeSize() {return 144;}
        static const uint32_t iKeyboardKeyOffset(int iKey) {return 144+iKey*16;}
        static const uint32_t iKeyboardKeySize()           {return 16;}
        static const uint32_t size()                       {return 4240;}

        static constexpr unsigned int nUniforms = 9;
        
        static constexpr const char* glslName = "SharedBlock";
        // The order of the uniforms within the block source must be the same as the
        // order in which they have been delcared in CPUBlock. On the other hand,
        // the actual uniform names do not matter, but I keep them mapped for
        // consistency
        static constexpr const char* glslSource =
R"(layout(std140) uniform SharedBlock {
        float iTime;
        int iFrame;
        int iRenderPass;
        bool iUserAction;
        vec3 iWASD;
        vec3 iLook;
        vec4 iMouse;
        mat4 iMVP;
        float iWindowAspectRatio;
        vec2 iWindowResolution;
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
          vir::Camera*        shaderCamera_   = nullptr;

          std::unordered_map<Uniform::SpecialType, glm::vec2> 
                              bounds_         = {};

    void setUserAction(bool flag);
    void setResolution(glm::ivec2& resolution, bool windowFrameManuallyDragged);
    void toggleMouseInputs();
    void toggleKeyboardInputs();
    void toggleCameraKeyboardInputs();
    void toggleCameraMouseInputs();

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

    const bool& isRenderingPaused() const {return flags_.isRenderingPaused;}
    const float& iTime() const {return cpuBlock_.iTime;}

    const char* glslBlockSource() const {return cpuBlock_.glslSource;}

    void bindShader(vir::Shader* shader) const;

    void update(const UpdateArgs& args);
    
    void renderWindowResolutionMenuGUI();
};

}