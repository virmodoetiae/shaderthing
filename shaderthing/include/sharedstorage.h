/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include "thirdparty/glm/glm.hpp"

#include "shaderthing/include/macros.h"

namespace vir
{
class Shader;
class ShaderStorageBuffer;
}

namespace ShaderThing
{

class ObjectIO;

class SharedStorage
{
    struct Block
    {
        #define                      SHARED_STORAGE_ARRAY_SIZE 4096
        const void*                  dataStart  = nullptr;
        const int*                   ioIntData  = nullptr;
        const glm::vec4*             ioVec4Data = nullptr;
        static constexpr const int   arraySize  = SHARED_STORAGE_ARRAY_SIZE;
        static constexpr const int   size       = arraySize*
                                                  (
                                                    sizeof(float)+
                                                    sizeof(glm::vec4)
                                                  );
        static constexpr const char* glslName   = "sharedStorageBlock";
        static constexpr const char* glslSource =
//  Using std430 to avoid packing problems, plus, if ShaderThing is running on a
//  computer with an OpenGL version < 4.3, the SSBO is not available anyway, so
//  no real compatibility breaking
"layout(std430) coherent buffer sharedStorageBlock {\n"
"int    ioIntData[" TO_STRING(SHARED_STORAGE_ARRAY_SIZE) "];"
" // <- atomically writeable\n"
"vec4   ioVec4Data[" TO_STRING(SHARED_STORAGE_ARRAY_SIZE) "];};\n";
    };

    struct GUI
    {
        bool isOpen                          = false;
        bool isDetachedFromMenu              = false;
        bool isVec4DataAlsoShownAsColor      = false;
        int  ioIntDataViewStartIndex         = 0;
        int  ioIntDataViewEndIndex           = 7;
        int  ioVec4DataViewStartIndex        = 0;
        int  ioVec4DataViewEndIndex          = 7;
        int  ioVec4DataViewPrecision         = 3;
        bool ioVec4DataViewExponentialFormat = false;
        bool ioVec4DataViewComponents[4]     = {true, true, true, true};
        std::string ioVec4DataViewFormat;
    };
    
    Block                     block_        = {};
    vir::ShaderStorageBuffer* buffer_       = nullptr;
    GUI                       gui_          = {};
    bool                      isSupported_  = false;
    const unsigned int        bindingPoint_ = 0;

public:
    
    SharedStorage();
    ~SharedStorage();
    
    void save(ObjectIO& io) const;
    static SharedStorage* load(const ObjectIO& io);
    
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