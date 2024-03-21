#pragma once

#include "thirdparty/glm/glm.hpp"

namespace vir
{
class Shader;
class ShaderStorageBuffer;
}

namespace ShaderThing
{

class SharedStorage
{
    struct Block
    {
        const void*                  dataStart  = nullptr;
        const int*                   ioIntData  = nullptr;
        const glm::vec4*             ioVec4Data = nullptr;
        static constexpr const int   arraySize  = 1024;
        static constexpr const int   size       = arraySize*(sizeof(float)+sizeof(glm::vec4));
        static constexpr const char* glslName   = "sharedStorageBlock";
        static constexpr const char* glslSource =
//  Using std430 to avoid packing problems, plus, if ShaderThing is running on a
//  computer with an OpenGL version < 4.3, the SSBO is not available anyway, so
//  no real compatibility breaking
R"(layout(std430) coherent buffer sharedStorageBlock {
int    ioIntData[1024]; // <- atomically writeable!
vec4   ioVec4Data[1024];};
)";
    };

    struct GUI
    {
        bool isOpen             = false;
        bool isDetachedFromMenu = false;
        int  ioIntDataViewStartIndex  = 0;
        int  ioIntDataViewEndIndex    = 7;
        int  ioVec4DataViewStartIndex = 0;
        int  ioVec4DataViewEndIndex   = 7;
    };
    
    Block                     block_        = {};
    vir::ShaderStorageBuffer* buffer_       = nullptr;
    GUI                       gui_          = {};
    bool                      isSupported_  = false;
    const unsigned int        bindingPoint_ = 0;

public:
    
    SharedStorage();
    ~SharedStorage();
    
    // Reset block contents to 0s
    void clear();
    
    void bindShader(vir::Shader* shader);

    // Call after every rendering call to sync read/write operations to the SSBO
    // buffer_ by different shader invocations (i.e., within GLSL fragment 
    // shader code)
    void gpuMemoryBarrier() const;
    
    // Call before every read/write operation of mapped Block data to ensure that
    // the latest changes by the shaders to the SSBO buffer_ data are visibile
    // on the CPU side
    void cpuMemoryBarrier() const ;

    const char* glslBlockSource() const;

    void renderGui();
    void renderMenuItemGui();

    bool isGuiOpen() const {return gui_.isOpen;};
    bool isGuiDetachedFromMenu() const {return gui_.isDetachedFromMenu;};
};

}