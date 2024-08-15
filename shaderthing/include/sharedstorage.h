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

#include <type_traits>

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
    //--------------------------------------------------------------------------
    struct Block
    {
    public:
        enum class IntType
        {
            I32 = 0,
            I64 = 1,
            UI32 = 2,
            UI64 = 3
        };
        enum class FloatType
        {
            F32 = 0,
            F64 = 1 // I.e., double
        };
    protected:
        IntType            intType_        = IntType::I32;
        FloatType          floatType_      = FloatType::F64;
        const unsigned int intDataSize_;
        const unsigned int floatDataSize_;
    public:
        Block
        (
            unsigned int intDataSize = 2048, 
            unsigned int floatDataSize = 2097152
        ) :
            intDataSize_(intDataSize),
            floatDataSize_(floatDataSize){}
        virtual ~Block() = default;

        IntType intType() const {return intType_;}
        FloatType floatType() const {return floatType_;}
        unsigned int intDataSize() const {return intDataSize_;}
        unsigned int floatDataSize() const {return floatDataSize_;}

        virtual unsigned int size() const = 0;
        virtual const char* intFormat() const = 0;
        virtual unsigned int nFloatComponents() const = 0;
        virtual std::string glslSource() const = 0;
        virtual void clear() = 0;
        virtual void printInt
        (
            void (*func)(const char* fmt, ...), 
            unsigned int index
        ) const = 0;
        virtual void printFloat
        (
            void (*func)(const char* fmt, ...), 
            unsigned int index, 
            unsigned int cmpt, 
            const char* format
        ) const = 0;
        virtual void printFloatAsColor
        (
            bool (*func)(const char* label, float* col, int flags),
            unsigned int index,
            int flags
        ) const = 0;
        static constexpr const char* glslName = "sharedStorageBlock";
    };

    //--------------------------------------------------------------------------
    template<typename T_IntType, typename T_FloatType, unsigned int T_NFloatCmpts>
    struct TypedBlock : public Block
    {
        using T_VecType = typename std::conditional
        <
            (T_NFloatCmpts > 1),
            glm::vec<T_NFloatCmpts, T_FloatType, glm::packed_highp>,
            T_FloatType
        >::type;

        TypedBlock
        (
            unsigned int intDataSize = 2048, 
            unsigned int floatDataSize = 2097152
        ) :
            Block(intDataSize, floatDataSize)
        {
            
            if (std::is_same<T_IntType, int32_t>::value)
                intType_ = IntType::I32;
            else if (std::is_same<T_IntType, int64_t>::value)
                intType_ = IntType::I64;
            else if (std::is_same<T_IntType, uint32_t>::value)
                intType_ = IntType::UI32;
            else if (std::is_same<T_IntType, uint64_t>::value)
                intType_ = IntType::UI64;
            else
                throw std::logic_error(
"SharedStorge::Block - First template argument must be a 32- or 64-bit signed"
"or unsigned integer type");
            if (std::is_same<T_FloatType, float>::value)
                floatType_ = FloatType::F32;
            else if (std::is_same<T_FloatType, double>::value)
                floatType_ = FloatType::F64;
            else
                throw std::logic_error(
"SharedStorge::Block - Second template argument must be a float or double type");
            
        }

        const void*      dataStart  = nullptr;
        const T_IntType* intData  = nullptr;
        const T_VecType* floatData  = nullptr;

        static constexpr const char* glslName   = "sharedStorageBlock";

        void initialize(vir::ShaderStorageBuffer* buffer)
        {
            dataStart = buffer->mapData();
            intData = (T_IntType*)dataStart;
            floatData = (T_VecType*)(intData+intDataSize_);
        }
        
        unsigned int size() const override 
        {
            return 
                intDataSize_*sizeof(T_IntType)+
                floatDataSize_*sizeof(T_VecType);
        }
        
        const char* intFormat() const override
        {
            if (std::is_same<T_IntType, int32_t>::value)
                return "%d";
            if (std::is_same<T_IntType, int64_t>::value)
                return "%lld";
            if (std::is_same<T_IntType, uint32_t>::value)
                return "%u";
            if (std::is_same<T_IntType, uint64_t>::value)
                return "%llu";
            return "";
        }
        
        virtual unsigned int nFloatComponents() const override
        {
            return T_NFloatCmpts;
        }
        
        std::string glslSource() const override
        {
            std::string sIntType;
            if (std::is_same<T_IntType, int32_t>::value)
                sIntType = "int";
            else if (std::is_same<T_IntType, int64_t>::value)
                sIntType = "int64_t";
            else if (std::is_same<T_IntType, uint32_t>::value)
                sIntType = "uint";
            else if (std::is_same<T_IntType, uint64_t>::value)
                sIntType = "uint64_t";
            std::string sVecType;
            if (std::is_same<T_FloatType, float>::value)
                sVecType = T_NFloatCmpts > 1 ? "vec" : "float";
            else if (std::is_same<T_FloatType, double>::value)
                sVecType = T_NFloatCmpts > 1 ? "dvec" : "double";
            if (T_NFloatCmpts > 1)
                sVecType += std::to_string(T_NFloatCmpts);
            //  Using std430 to avoid packing problems, plus, if ShaderThing is
            //  running on a computer with an OpenGL version < 4.3, the SSBO is
            //  not available anyway, so no real compatibility breaking
            return
                "layout(std430) coherent buffer sharedStorageBlock {\n        "+
                sIntType+" ssiData["+std::to_string(intDataSize_)+"];\n        "+
                sVecType+" ssfData[];}; // Dynamic size up to "+
                std::to_string(floatDataSize_)+"\n";
        }
        
        void printInt
        (
            void (*func)(const char* fmt, ...), 
            unsigned int index
        ) const override
        {
            func(intFormat(), intData[index]);
        }
        
        void printFloat
        (
            void (*func)(const char* fmt, ...), 
            unsigned int index, 
            unsigned int cmpt, 
            const char* format
        ) const override
        {
            if constexpr (T_NFloatCmpts > 1)
                func(format, floatData[index][cmpt]);
            else
                func(format, floatData[index]);
        }

        virtual void printFloatAsColor
        (
            bool (*func)(const char* label, float* color, int flags),
            unsigned int index,
            int flags
        ) const override
        {
            if constexpr (T_NFloatCmpts > 2)
            {
                T_VecType value(floatData[index]);
                func("##floatDataColorViewer", (float*)(&(value.x)), flags);
            }
        }
        
        void clear() override
        {
            auto dataStart = (unsigned char*)dataStart;
            for (unsigned int i=0; i<size(); i++)
                *(dataStart+i) = 0;
        }
    };

    //------------------------------------------------------------------------//

    struct GUI
    {
        bool isIconSet                       = false;
        bool isDocked                        = false;
        bool isOpen                          = false;
        bool isDetachedFromMenu              = true;
        bool isFloatDataAlsoShownAsColor     = false;
        unsigned int intDataViewStartIndex   = 0;
        unsigned int intDataViewEndIndex     = 7;
        unsigned int floatDataViewStartIndex = 0;
        unsigned int floatDataViewEndIndex   = 7;
        unsigned int floatDataViewPrecision  = 3;
        bool floatDataViewExponentialFormat  = false;
        bool floatDataViewComponents[4]      = {true, true, true, true};
        std::string floatDataViewFormat;
    };
    
    Block*                    block_         = nullptr;
    vir::ShaderStorageBuffer* buffer_        = nullptr;
    GUI                       gui_           = {};
    bool                      isSupported_   = false;
    const unsigned int        bindingPoint_  = 0;

public:
    
    SharedStorage();
    ~SharedStorage();
    
    void resetBlockAndSSBO
    (
        Block::IntType intType, 
        Block::FloatType floatType,
        const unsigned int nFloatComponents=4,
        const unsigned int intDataSize = 2048,
        const unsigned int floatDataSize = 2097152
    );

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