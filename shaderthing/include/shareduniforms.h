/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "shaderthing/include/macros.h"
#include "shaderthing/include/uniform.h"

#include "vir/include/vir.h"

#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

class ObjectIO;
class Random;
class Resource;

class SharedUniforms : vir::Event::Receiver
{

friend Uniform;

public :

    struct UpdateArgs
    {
        const bool advanceFrame;
        const float timeStep;
    };

private:

    struct Flags
    {
              bool updateDataRangeII                  = false;
              bool stepToNextFrame                    = false;
              bool stepToNextTimeStep                 = false;
              bool resetFrameCounter                  = true;
              bool resetFrameCounterPreOrPostExport   = true;
              bool isRenderingPaused                  = false;
              bool isTimePaused                       = false;
              bool isTimePausedBecauseRenderingPaused = false;
              bool isTimeLooped                       = false;
              bool isTimeResetOnFrameCounterReset     = true;
              bool isTimeDeltaSmooth                  = false;
              bool isRandomNumberGeneratorPaused      = false;
              bool isKeyboardInputEnabled             = true; // iKeyboard
              bool isMouseInputEnabled                = true; // iMouse
              bool isMouseInputClampedToWindow        = false;
              bool mouseInputRequiresLMBHold          = true;
              bool isCameraKeyboardInputEnabled       = true; // iWASD
              bool isCameraMouseInputEnabled          = true; // iLook
              bool cameraMouseInputRequiresLMBHold    = true;
              bool isVSyncEnabled                     = true;
              bool isSSBOSupported                    = false;
    };

    struct ExportData
    {
        float      originalTime;
        glm::ivec2 originalResolution;
        glm::ivec2 resolution;
        float      resolutionScale               = 1.f;
    };

    // A properly aligned layout-std140 compilant C++ representation of the
    // ShaderThing fragment uniform block. Grouped by update frequency for 
    // convenience of passing only certain ranges of it to the GPU UniformBlock
    // at a time. Somewhat wasteful because of the extensive usage of vec3s
    struct FragmentBlock
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
                    int        iFrame       = 0;            //  0 -  4       | 4
                    int        iRenderPass  = 0;            //  4 -  8       | 4
                    float      iTime        = 0.f;          //  8 - 12       | 4
                    float      iTimeDelta   = 0.f;          // 12 - 16       | 4
                    float      iRandom      = 0.f;          // 16 - 20       | 4
        // Medium update frequency (data range II)
                    bool       iUserAction  = false;        // 20 - 21       | 1
        alignas(4)  bool       iExport      = false;        // 24 - 25       | 1
                    vec3A16    iWASD        = {0,0,-1};     // 32 - 48       | 16
                    vec3A16    iLook        = {0,0,1};      // 48 - 64       | 16
                    glm::vec4  iMouse       = {0,0,0,0};    // 64 - 80       | 16
        // Low update frequency (data range III)
                    float      iAspectRatio = 1.f;          // 80 - 84       | 4
        alignas(8)  glm::vec2  iResolution  = {512,512};    // 88 - 96       | 8

        // Whole array never updated all at once, only one 
        // array element at a time, medium update frequency
                    ivec3A16   iKeyboard[256] {};           // 96 - 4192     | 16*256 = 4096

        static      uint32_t dataRangeISize()             {return 20;}
        static      uint32_t dataRangeIISize()            {return 80;}
        static      uint32_t dataRangeIIISize()           {return 96;}
        static      uint32_t iKeyboardKeyOffset(int iKey) {return 96+iKey*16;}
        static      uint32_t iKeyboardKeySize()           {return 16;}
        static      uint32_t size()                       {return 4192;}
        
        static constexpr const char* glslName = "sharedBlock";
        // The order of the uniforms within the block source must be the same as
        // the order in which they have been delcared in FragmentBlock. On the
        // other hand, the actual uniform names do not matter
        static constexpr const char* glslSource =
R"(layout(std140) uniform sharedUniformBlock {
        int    iFrame;
        int    iRenderPass;
        float  iTime;
        float  iTimeDelta;
        float  iRandom;
        bool   iUserAction;
        bool   iExport;
        vec3   iWASD;
        vec3   iLook;
        vec4   iMouse;
        float  iWindowAspectRatio;
        vec2   iWindowResolution;
        ivec3  iKeyboard[256];};
)";
    };

    struct VertexBlock
    {
        glm::mat4 iMVP = glm::mat4(0);
        static uint32_t size() {return 64;}
        static constexpr const char* glslName = "vertexUniformBlock";
        static constexpr const char* glslSource =
R"(layout(std140) uniform vertexUniformBlock {mat4 iMVP;};
)";
    };
    
    const unsigned int        fBindingPoint_ = 0;
    const unsigned int        vBindingPoint_ = 1;
          FragmentBlock       fBlock_        = {};
          VertexBlock         vBlock_        = {};
          vir::UniformBuffer* fBuffer_       = nullptr;
          vir::UniformBuffer* vBuffer_       = nullptr;
          Flags               flags_         = {};
          ExportData          exportData_    = {};
        
          // Fixed camera used to retrieve the value of the projection view 
          // matrix iMVP
          vir::Camera*        screenCamera_   = nullptr;

          // Movable camera which responds to keyboard and mouse controls and is
          // used to provide values to cpuBlock.iWASD, cpuBlock.iLook
          vir::Camera*        shaderCamera_   = nullptr;

          std::unordered_map<Uniform::SpecialType, glm::vec2> 
                              bounds_         = {};

          // List of user-created uniforms which are shared by all layers
          std::vector<Uniform*> 
                              userUniforms_   = {};

          // For generating random numbers for iSeed
          Random*             random_         = nullptr;

    // Only used in the post-loading step
    struct Cache
    {
        std::map<Uniform*, std::string> 
                              uninitializedResourceLayers = {};
    };
          Cache               cache_ = {};

    void setUserAction(bool flag);
    void setResolution
    (
        glm::ivec2& resolution, 
        bool windowFrameManuallyDragged, 
        bool prepareForExport=false
    );
    void setMouseInputsClamped(bool flag);
    void toggleMouseInputs();
    void toggleKeyboardInputs();
    void toggleCameraKeyboardInputs();
    void toggleCameraMouseInputs();
    void setMouseCaptured(bool flag);

    DELETE_COPY_MOVE(SharedUniforms)

public:

    SharedUniforms();
    ~SharedUniforms();

    void save(ObjectIO& io) const;
    static void load
    (
        const ObjectIO& io, 
        SharedUniforms*& SharedUniforms,
        const std::vector<Resource*>& resources
    );
    void postLoadProcessCachedResourceLayers
    (
        const std::vector<Resource*>& resources
    );

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

    void resetFrameCounter() {flags_.resetFrameCounter = true;}

    void bindShader(vir::Shader* shader) const;
    void update(const UpdateArgs& args);
    void nextRenderPass(unsigned int nMaxRenderPasses=1);
    void prepareForExport(bool setTime, float exportStartTime);
    void resetAfterExport(bool resetFrameCounter = true);
    void resetTimeAndFrame(float time=0);
    void toggleRenderingPaused();

    void renderWindowMenuGui();

    const char* glslFragmentBlockSource() const {return fBlock_.glslSource;}
    const char* glslVertexBlockSource() const {return vBlock_.glslSource;}

    ExportData& exportData() {return exportData_;}
    
    const bool& stepToNextFrame() const {return flags_.stepToNextFrame;}
    const bool& isRenderingPaused() const {return flags_.isRenderingPaused;}
    const bool& isTimeDeltaSmooth() const {return flags_.isTimeDeltaSmooth;}
    const float& iTime() const {return fBlock_.iTime;}
    const int& iFrame() const {return fBlock_.iFrame;}
    const int& iRenderPass() const {return fBlock_.iRenderPass;}
    glm::ivec2 iResolution() const {return fBlock_.iResolution;}
    const std::vector<Uniform*>& userUniforms() const {return userUniforms_;}
};

}