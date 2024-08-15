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
    template<typename intType, typename floatType>
    struct Block
    {
        typedef glm::vec<4, floatType, glm::packed_highp> vec4Type;
        #define                      SHARED_STORAGE_INT_ARRAY_SIZE 2048
                                                                 // 2048*1024
        #define                      SHARED_STORAGE_VEC4_ARRAY_SIZE 2097152
        const void*                  dataStart  = nullptr;
        const intType*               ioIntData  = nullptr;
        const vec4Type*              ioVec4Data = nullptr;
        static constexpr const int   size       = SHARED_STORAGE_INT_ARRAY_SIZE
                                                  *sizeof(intType)+
                                                  SHARED_STORAGE_VEC4_ARRAY_SIZE
                                                  *sizeof(vec4Type);
        static constexpr const int   intSize    = sizeof(intType);
        static constexpr const int   vec4Size   = sizeof(vec4Type);
        static constexpr const char* glslName   = "sharedStorageBlock";
        static constexpr const char* intFormat()
        {
            if (std::is_same<intType, int32_t>::value)
                return "%d";
            if (std::is_same<intType, int64_t>::value)
                return "%lld";
            if (std::is_same<intType, uint32_t>::value)
                return "%u";
            if (std::is_same<intType, uint64_t>::value)
                return "%llu";
            throw std::logic_error(
"SharedStorge::Block - First template argument must be a 32- or 64-bit signed"
"or unsigned integer type");
        }
        //  Using std430 to avoid packing problems, plus, if ShaderThing is running on a
        //  computer with an OpenGL version < 4.3, the SSBO is not available anyway, so
        //  no real compatibility breaking
        static constexpr std::string glslSource() 
        {
            std::string sIntType;
            if (std::is_same<intType, int32_t>::value)
                sIntType = "int     ";
            else if (std::is_same<intType, int64_t>::value)
                sIntType = "int64_t ";
            else if (std::is_same<intType, uint32_t>::value)
                sIntType = "uint    ";
            else if (std::is_same<intType, uint64_t>::value)
                sIntType = "uint64_t";
            else
                throw std::logic_error(
"SharedStorge::Block - First template argument must be a 32- or 64-bit signed"
"or unsigned integer type");
            std::string sVec4Type;
            if (std::is_same<floatType, float>::value)
                sVec4Type = "vec4    ";
            else if (std::is_same<floatType, double>::value)
                sVec4Type = "dvec4   ";
            else
                throw std::logic_error(
"SharedStorge::Block - Second template argument must be a float or double type");
            return
"layout(std430) coherent buffer sharedStorageBlock {\n        "+
sIntType+" ioIntData["TO_STRING(SHARED_STORAGE_INT_ARRAY_SIZE)"];\n        "+
sVec4Type+" ioVec4Data[];}; // Dynamic size up to 2097152 (==2048*1024)\n";
        }
        void initialize(vir::ShaderStorageBuffer* buffer)
        {
            dataStart = buffer->mapData();
            ioIntData = (intType*)dataStart;
            ioVec4Data = 
                (vec4Type*)(ioIntData+SHARED_STORAGE_INT_ARRAY_SIZE*intSize);
        }
    };

    struct GUI
    {
        bool isIconSet                       = false;
        bool isDocked                        = false;
        bool isOpen                          = false;
        bool isDetachedFromMenu              = true;
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
    
    Block<uint32_t, float>    block_        = {};
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

    std::string glslBlockSource() const;

    void renderGui();
    void renderMenuItemGui();

    bool isGuiOpen() const {return gui_.isOpen;};
    bool isGuiDetachedFromMenu() const {return gui_.isDetachedFromMenu;};
};

}