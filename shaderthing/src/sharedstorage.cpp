/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/sharedstorage.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

void SharedStorage::resetBlockAndSSBO
(
    Block::IntType intType, 
    Block::FloatType floatType,
    const unsigned int nFloatComponents,
    const unsigned int intDataSize,
    const unsigned int floatDataSize
)
{
    //if (block_ != nullptr)
    //    delete block_;
    //if (buffer_ != nullptr)
    //    delete buffer_;

    // Preprocessor madness to have dynamic types for the TypedBlock (4*2*4 = 32
    // different combinations)
    //-------------------------------------
#define INITIALIZE_BLOCK_AND_SSBO_0(I, F, N)                                \
{                                                                           \
    auto typedBlock = new TypedBlock<I, F, N>(intDataSize, floatDataSize);  \
    buffer_ = vir::ShaderStorageBuffer::create(typedBlock->size());         \
    buffer_->bind();                                                        \
    buffer_->setBindingPoint(bindingPoint_);                                \
    typedBlock->initialize(buffer_);                                        \
    block_ = typedBlock;                                                    \
    break;                                                                  \
}
    //-------------------------------------
#define INITIALIZE_BLOCK_AND_SSBO_1(I, F, N)                                \
switch(I)                                                                   \
{                                                                           \
case Block::IntType::I32 :                                                  \
    INITIALIZE_BLOCK_AND_SSBO_0(int32_t, F, N)                              \
case Block::IntType::I64 :                                                  \
    INITIALIZE_BLOCK_AND_SSBO_0(int64_t, F, N)                              \
case Block::IntType::UI32 :                                                 \
    INITIALIZE_BLOCK_AND_SSBO_0(uint32_t, F, N)                             \
case Block::IntType::UI64 :                                                 \
    INITIALIZE_BLOCK_AND_SSBO_0(uint64_t, F, N)                             \
}                                                                           \
break;
    //-------------------------------------
#define INITIALIZE_BLOCK_AND_SSBO_2(I, F, N)                                \
switch(F)                                                                   \
{                                                                           \
case Block::FloatType::F32 :                                                \
    INITIALIZE_BLOCK_AND_SSBO_1(I, float, N)                                \
case Block::FloatType::F64 :                                                \
    INITIALIZE_BLOCK_AND_SSBO_1(I, double, N)                               \
}                                                                           \
break;
    //-------------------------------------
    // No 3-component variant because of memory alignment issues: it would
    // occupy as much space a 4-component-based array, so just use that instead
#define INITIALIZE_BLOCK_AND_SSBO(I, F, N)                                  \
switch(N)                                                                   \
{                                                                           \
case 1 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 1)                                    \
case 2 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 2)                                    \
case 4 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 4)                                    \
default :                                                                   \
    break;                                                                  \
}                                                                           \
    //-------------------------------------
    
    INITIALIZE_BLOCK_AND_SSBO(intType, floatType, nFloatComponents)
    
    // Changing type and/or resizing may turn some of the data in memory
    // into garbage, so make sure to clean up the storage
    block_->clear();
}

//----------------------------------------------------------------------------//

SharedStorage::SharedStorage()
{
    isSupported_ = vir::ShaderStorageBuffer::create(1)->canRunOnDeviceInUse();
    if (isSupported_)
    {
        resetBlockAndSSBO(Block::IntType::I32, Block::FloatType::F32);
        gui_.floatDataViewFormat =
        (
            "%."+
            std::to_string(gui_.floatDataViewPrecision)+
            (gui_.floatDataViewExponentialFormat ? "e" : "f")
        );
    }
}

//----------------------------------------------------------------------------//

UPtr<SharedStorage> SharedStorage::create()
{
    return UPtr<SharedStorage>(new SharedStorage());
}

//----------------------------------------------------------------------------//

SharedStorage::~SharedStorage()
{
    if (buffer_.valid())
    {
        buffer_->unbind();
        //delete buffer_;
    }
    DELETE_IF_NOT_NULLPTR(block_)
}

//----------------------------------------------------------------------------//

void SharedStorage::saveTo(ObjectIO& io) const
{
    if (!isSupported_)
        return;
    
    io.writeObjectStart("sharedStorage");

    #define WRITE_GUI_ITEM(Name)             \
        io.write(TO_STRING(Name), gui_.Name);

    WRITE_GUI_ITEM(intDataViewStartIndex)
    WRITE_GUI_ITEM(intDataViewEndIndex)
    WRITE_GUI_ITEM(floatDataViewEndIndex)
    WRITE_GUI_ITEM(floatDataViewStartIndex)
    WRITE_GUI_ITEM(isFloatDataAlsoShownAsColor)
    WRITE_GUI_ITEM(floatDataViewPrecision)
    WRITE_GUI_ITEM(floatDataViewExponentialFormat)
    glm::ivec4 values(4);
    for (int i=0; i<4; i++)
        values[i] = int(gui_.floatDataViewComponents[i]);
    io.write("floatDataViewComponents", values);

    #define WRITE_BLOCK_ITEM(Name)             \
        io.write(TO_STRING(Name), block_->Name());
    #define WRITE_BLOCK_ITEM_AS(Name, Type)             \
        io.write(TO_STRING(Name), (Type)block_->Name());
    
    WRITE_BLOCK_ITEM_AS(intType, unsigned int)
    WRITE_BLOCK_ITEM_AS(floatType, unsigned int)
    WRITE_BLOCK_ITEM(intDataSize)
    WRITE_BLOCK_ITEM(floatDataSize)
    WRITE_BLOCK_ITEM(nFloatComponents)

    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

UPtr<SharedStorage> SharedStorage::loadFrom(const ObjectIO& io)
{
    auto sharedStorage = SharedStorage::create();
    if (!io.hasMember("sharedStorage") || !sharedStorage->isSupported_)
        return sharedStorage;

    auto ioSS = io.readObject("sharedStorage");
    auto gui = GUI{};

#define READ_GUI_ITEM(Name, Type)                                           \
    gui.Name = ioSS.readOrDefault<Type>(TO_STRING(Name), gui.Name);

    READ_GUI_ITEM(intDataViewStartIndex, int)
    READ_GUI_ITEM(intDataViewEndIndex, int)
    READ_GUI_ITEM(floatDataViewEndIndex, int)
    READ_GUI_ITEM(floatDataViewStartIndex, int)
    READ_GUI_ITEM(isFloatDataAlsoShownAsColor, bool)
    READ_GUI_ITEM(floatDataViewPrecision, int)
    READ_GUI_ITEM(floatDataViewExponentialFormat, bool)
    glm::ivec4 values = 
        ioSS.readOrDefault<glm::ivec4>
        (
            "floatDataViewComponents", 
            glm::ivec4{1, 1, 1, 1}
        );
    for (int i=0; i<4; i++)
        gui.floatDataViewComponents[i] = bool(values[i]);
    gui.floatDataViewFormat =
    (
        "%."+
        std::to_string(gui.floatDataViewPrecision)+
        (gui.floatDataViewExponentialFormat ? "e" : "f")
    );
    sharedStorage->gui_ = gui;

#define READ_BLOCK_ITEM(Name, Type)                                         \
    Type Name = ioSS.readOrDefault<Type>(TO_STRING(Name),                   \
                                  (Type)sharedStorage->block_->Name());

    READ_BLOCK_ITEM(intType, unsigned int)
    READ_BLOCK_ITEM(floatType, unsigned int)
    READ_BLOCK_ITEM(intDataSize, unsigned int)
    READ_BLOCK_ITEM(floatDataSize, unsigned int)
    READ_BLOCK_ITEM(nFloatComponents, unsigned int)

    sharedStorage->resetBlockAndSSBO
    (
        (Block::IntType)intType,
        (Block::FloatType)floatType,
        nFloatComponents,
        intDataSize,
        floatDataSize
    );

    return sharedStorage;
}

//----------------------------------------------------------------------------//

void SharedStorage::clear()
{
    if (!isSupported_)
        return;
    buffer_->fenceSync();
    block_->clear();
}

//----------------------------------------------------------------------------//

void SharedStorage::gpuMemoryBarrier() const
{
    if (isSupported_)
        buffer_->memoryBarrier();
}

//----------------------------------------------------------------------------//

void SharedStorage::cpuMemoryBarrier() const
{
    if (isSupported_)
        buffer_->fenceSync();
}

//----------------------------------------------------------------------------//

void SharedStorage::bindShader(vir::Shader* shader)
{
    if (isSupported_)
        shader->bindShaderStorageBlock(block_->glslName, bindingPoint_);
}

//----------------------------------------------------------------------------//

std::string SharedStorage::shaderSource() const
{
    if (isSupported_)
        return block_->glslSource();
    return "";
}

}