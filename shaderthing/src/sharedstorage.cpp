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

#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/objectio.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

void SharedStorage::resetBlockAndSSBO
(
    Block::IntType intType, 
    Block::FloatType floatType,
    const unsigned int nFloatComponents,
    const unsigned int intDataSize,
    const unsigned int floatDataSize
)
{
    if (block_ != nullptr)
        delete block_;
    if (buffer_ != nullptr)
        delete buffer_;

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
#define INITIALIZE_BLOCK_AND_SSBO(I, F, N)                                  \
switch(N)                                                                   \
{                                                                           \
case 1 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 1)                                    \
case 2 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 2)                                    \
case 3 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 3)                                    \
case 4 :                                                                    \
    INITIALIZE_BLOCK_AND_SSBO_2(I, F, 4)                                    \
default :                                                                   \
    break;                                                                  \
}                                                                           \
    //-------------------------------------
    
    INITIALIZE_BLOCK_AND_SSBO(intType, floatType, nFloatComponents)
}

SharedStorage::SharedStorage()
{
    auto isSupported = [&]()
    {
        bool result = false;
        auto* ssbo = vir::ShaderStorageBuffer::create(1);
        result = ssbo->canRunOnDeviceInUse();
        delete ssbo;
        return result;
    };
    isSupported_ = isSupported();
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

SharedStorage::~SharedStorage()
{
    if (buffer_ != nullptr)
    {
        buffer_->unbind();
        delete buffer_;
    }
    DELETE_IF_NOT_NULLPTR(block_)
}

//----------------------------------------------------------------------------//

void SharedStorage::save(ObjectIO& io) const
{
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

SharedStorage* SharedStorage::load(const ObjectIO& io)
{
    auto sharedStorage = new SharedStorage();
    if (!io.hasMember("sharedStorage"))
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

std::string SharedStorage::glslBlockSource() const
{
    if (isSupported_)
        return block_->glslSource();
    return "";
}

//----------------------------------------------------------------------------//

bool SharedStorage::renderGui()
{
    if (!gui_.isOpen)
        return false;
    if (!isSupported_)
    {
        ImGui::Text
        (
            "Not supported due to OpenGL version <= 4.2, requires >= 4.3"
        );
        return false;
    }
    bool shadersRequireRecompilation = false;
    const float fontSize = ImGui::GetFontSize();
    const float textWidth = 45.f*fontSize;
    if (gui_.isDetachedFromMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(768, 512), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Shared storage view", &gui_.isOpen, windowFlags);

        // Refresh icon if needed
        if (!gui_.isIconSet || gui_.isDocked != ImGui::IsWindowDocked())
        {
            gui_.isIconSet = vir::ImGuiRenderer::setWindowIcon
            (
                "Shared storage view", 
                ByteData::Icon::sTIconData, 
                ByteData::Icon::sTIconSize,
                false
            );
            gui_.isDocked = ImGui::IsWindowDocked();
        }
    }
    else
        ImGui::Dummy({textWidth, 0});

    static const std::unordered_map<Block::IntType, const char*> 
        intTypeToName =
        {
            {Block::IntType::UI64, "Uint 64-bit"},
            {Block::IntType::UI32, "Uint 32-bit"},
            {Block::IntType::I64, "Int 64-bit"},
            {Block::IntType::I32, "Int 32-bit"},
        };
    static auto intType = block_->intType();
    static auto floatType = block_->floatType();
    static auto nFloatComponents = block_->nFloatComponents();
    static auto intDataSize = block_->intDataSize();
    static auto floatDataSize = block_->floatDataSize();
    ImGui::Text("ssiData type       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::BeginCombo
        (
            "##ssiDataTypeCombo", 
            intTypeToName.at(intType)
        )
    )
    {
        for (const auto& item : intTypeToName)
        {
            if (ImGui::Selectable(item.second))
                intType = item.first;
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::Text("ssiData size       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::InputInt("##ssiDataSize", (int*)(&intDataSize)))
        intDataSize = std::min(std::max(intDataSize, 1u), 4096u);
    ImGui::PopItemWidth();

    static const std::unordered_map<Block::FloatType, const char*> 
        floatTypeToName =
        {
            {Block::FloatType::F64, "Float 64-bit"},
            {Block::FloatType::F32, "Float 32-bit"}
        };
    ImGui::Text("ssfData type       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::BeginCombo
        (
            "##ssfDataTypeCombo", 
            floatTypeToName.at(floatType)
        )
    )
    {
        for (const auto& item : floatTypeToName)
        {
            if (ImGui::Selectable(item.second))
                floatType = item.first;
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::Text("ssfData size       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::InputInt("##ssfDataSize", (int*)(&floatDataSize)))
        floatDataSize = std::min(std::max(floatDataSize, 1u), 4194304u);
    ImGui::PopItemWidth();
    ImGui::Text("ssfData components ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::BeginCombo
        (
            "##ssfDataNComponentsCombo", 
            std::to_string(nFloatComponents).c_str()
        )
    )
    {
        for (unsigned int nCmpts = 1; nCmpts <= 4; nCmpts++)
        {
            if (ImGui::Selectable(std::to_string(nCmpts).c_str()))
                nFloatComponents = nCmpts;
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    bool disabled = true;
    if 
    (
        intType != block_->intType() ||
        intDataSize != block_->intDataSize() ||
        floatType != block_->floatType() ||
        floatDataSize != block_->floatDataSize() ||
        nFloatComponents != block_->nFloatComponents()
    )
        disabled = false;
    if (disabled)
        ImGui::BeginDisabled();
    if (ImGui::Button("Update shared storage buffer", {-1, 0}))
    {
        shadersRequireRecompilation = true;
        resetBlockAndSSBO
        (
            intType,
            floatType,
            nFloatComponents,
            intDataSize,
            floatDataSize
        );
        intType = block_->intType();
        floatType = block_->floatType();
        nFloatComponents = block_->nFloatComponents();
        intDataSize = block_->intDataSize();
        floatDataSize = block_->floatDataSize();
    }
    if (disabled)
        ImGui::EndDisabled();
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
        ImGui::BeginTooltip()
    )
    {
        std::cout << "A" << std::endl;
        double vSpace = ImGui::GetTextLineHeightWithSpacing();
        ImGui::Text("Important notes and tips:");
        ImGui::Dummy({-1, .25*vSpace});
        ImGui::Bullet(); ImGui::Text(
"updating the storage buffer automatically recompiles all layers;");
        ImGui::Dummy({-1, .25*vSpace});
        ImGui::Bullet(); ImGui::Text(
R"(using 64-bit types may require enabling additional OpenGL extensions from 
'Properties' -> 'OpenGL extensions'. The specific extension(s) to be enabled,
if any, will be dispalyed in the layer compilation error messages on storage
buffer update;)");
        ImGui::Dummy({-1, .25*vSpace});
        ImGui::Bullet(); ImGui::Text(
R"(in GLSL, 64-bit floating point numbers should be declared with the 'lf' 
suffix (e.g., 'double x = 1.389lf;'), else they might be interpreted as 32-
bit floating point numbers. Similarly, 64-bit signed and unsigned integers
should be suffixed by 'l' and 'ul' respectively (e.g., 'uint64_t x = 1389l;',
'int64_t x = 1389ul;'))");
        ImGui::EndTooltip();
    }
    
    float controlsHeight = 8*ImGui::GetTextLineHeightWithSpacing();
    {
        ImGui::BeginChild
        (
            "##sharedStorageIntControlsChild", 
            ImVec2(ImGui::GetContentRegionAvail().x * 0.30f, controlsHeight),
            false
        );
        ImGui::SeparatorText("ssiData");
        
        ImGui::Text("View start ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", (int*)(&gui_.intDataViewStartIndex)))
            gui_.intDataViewStartIndex = 
                std::max
                (
                    std::min
                    (
                        gui_.intDataViewStartIndex, 
                        gui_.intDataViewEndIndex
                    ),
                    0
                );
        ImGui::PopItemWidth();
        
        ImGui::Text("View end   ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##intEndIndex", (int*)(&gui_.intDataViewEndIndex));
            gui_.intDataViewEndIndex = 
                std::max
                (
                    std::min
                    (
                        gui_.intDataViewEndIndex, 
                        (int)block_->intDataSize()-1
                    ),
                    gui_.intDataViewStartIndex
                );
        ImGui::PopItemWidth();
        
        ImGui::EndChild();
    }
    ImGui::SameLine();
    {
        ImGui::BeginChild
        (
            "##sharedStorageVecControlsChild", 
            ImVec2(0, controlsHeight),
            false
        );
        ImGui::SeparatorText("ssfData");

        ImGui::Text("View start        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", (int*)(&gui_.floatDataViewStartIndex)))
            gui_.floatDataViewStartIndex = 
                std::max
                (
                    std::min
                    (
                        gui_.floatDataViewStartIndex, 
                        gui_.floatDataViewEndIndex
                    ),
                    0
                );
        ImGui::PopItemWidth();
        
        ImGui::Text("View end          ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##intEndIndex", (int*)(&gui_.floatDataViewEndIndex));
            gui_.floatDataViewEndIndex = 
                std::max
                (
                    std::min
                    (
                        gui_.floatDataViewEndIndex, 
                        (int)block_->floatDataSize()-1
                    ),
                    gui_.floatDataViewStartIndex
                );
        ImGui::PopItemWidth();

        ImGui::Text("Show color        ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##floatDataShowColor", 
            &gui_.isFloatDataAlsoShownAsColor
        );

        ImGui::Text("Precision, format ");
        ImGui::SameLine();
        ImGui::PushItemWidth(10*fontSize);
        if 
        (
            ImGui::SliderInt
            (
                "##floatDataPrecision", 
                (int*)(&gui_.floatDataViewPrecision), 
                0, 
                15
            )
        )
        {
            gui_.floatDataViewFormat = 
                "%."+
                std::to_string(gui_.floatDataViewPrecision)+
                (gui_.floatDataViewExponentialFormat ? "e" : "f");
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                gui_.floatDataViewExponentialFormat ?
                "Scientific" : "Decimal",
                {-1, 0}
            )
        )
        {
            gui_.floatDataViewExponentialFormat = 
                !gui_.floatDataViewExponentialFormat;
            gui_.floatDataViewFormat = 
                "%."+
                std::to_string(gui_.floatDataViewPrecision)+
                (gui_.floatDataViewExponentialFormat ? "e" : "f");
        }
        if (block_->nFloatComponents() > 1)
        {
            ImGui::Text("Show components   ");
            static const char* labels[4] = {"x ", "y ", "z ", "w"};
            for (int i=0; i<block_->nFloatComponents(); i++)
            {
                ImGui::SameLine();
                std::string label = "##floatDataShowCmpt"+std::to_string(i);
                ImGui::Checkbox(label.c_str(), &gui_.floatDataViewComponents[i]);
                ImGui::SameLine();
                ImGui::Text("%s", labels[i]);
            }
        }
        else
            gui_.floatDataViewComponents[0] = true;

        ImGui::EndChild();
    }

    ImGui::Separator();

    buffer_->fenceSync(); // Ensure all shaders have finished writing data

    float rangeViewerHeight = 
        gui_.isDetachedFromMenu ? 
        ImGui::GetContentRegionAvail().y-
        1.25*ImGui::GetTextLineHeightWithSpacing() :
        9*ImGui::GetTextLineHeightWithSpacing();
    float indexColumnWidth = 0;

    {
        ImGui::BeginChild
        (
            "##sharedStorageIntRangeViewerChild", 
            ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, rangeViewerHeight),
            false
        );
        if 
        (
            ImGui::BeginTable
            (
                "##sharedStorageIntDataTable", 
                2, 
                ImGuiTableFlags_BordersV | 
                ImGuiTableFlags_BordersH |
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollY,
                {0, 0}
            )
        )
        {
            indexColumnWidth = ImGui::GetContentRegionAvail().x/5;
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn
            (
                "Index", 
                0, 
                indexColumnWidth
            );
            ImGui::TableSetupColumn("Value", 0, -1);
            ImGui::TableHeadersRow();
            for 
            (
                int row = gui_.intDataViewStartIndex;
                row <= gui_.intDataViewEndIndex;
                row++
            )
            {
                ImGui::TableNextRow();
                ImGui::PushID(row);
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", row);
                ImGui::TableSetColumnIndex(1);
                block_->printInt(&ImGui::Text, row);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();
    {
        ImGui::BeginChild
        (
            "##sharedStorageVecRangeViewerChild", 
            ImVec2(0, rangeViewerHeight),
            false
        );
        if 
        (
            ImGui::BeginTable
            (
                "##sharedStorageVecDataTable", 
                2, 
                ImGuiTableFlags_BordersV | 
                ImGuiTableFlags_BordersH |
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollY,
                {0, 0}
            )
        )
        {
            ImGui::TableSetupColumn
            (
                "Index", 
                0, 
                indexColumnWidth
            );
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Value", 0, -1);
            ImGui::TableHeadersRow();
            for 
            (
                int row = gui_.floatDataViewStartIndex;
                row <= gui_.floatDataViewEndIndex;
                row++
            )
            {
                ImGui::TableNextRow();
                ImGui::PushID(row);
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", row);
                ImGui::TableSetColumnIndex(1);
                
                //const auto& value = block_.floatData[row];
                if (gui_.isFloatDataAlsoShownAsColor)
                {
                    if (block_->nFloatComponents() == 4)
                    {
                        block_->printFloatAsColor
                        (
                            &ImGui::ColorEdit4, 
                            row, 
                            ImGuiColorEditFlags_NoInputs
                        );
                        ImGui::SameLine();
                    }
                    else if (block_->nFloatComponents() == 3)
                    {
                        block_->printFloatAsColor
                        (
                            &ImGui::ColorEdit3, 
                            row, 
                            ImGuiColorEditFlags_NoInputs
                        );
                        ImGui::SameLine();
                    }
                }
                for (int i=0; i<block_->nFloatComponents(); i++)
                {
                    if (!gui_.floatDataViewComponents[i])
                        continue;
                    block_->printFloat
                    (
                        &ImGui::Text, 
                        row, 
                        i, 
                        gui_.floatDataViewFormat.c_str()
                    );
                    if (i < block_->nFloatComponents()-1)
                        ImGui::SameLine();
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
    if (ImGui::Button("Clear storage buffer", {-1, 0}))
        clear();
    if (gui_.isDetachedFromMenu)
        ImGui::End();
    return shadersRequireRecompilation;
}

//----------------------------------------------------------------------------//

bool SharedStorage::renderMenuItemGui()
{
    bool shadersRequireRecompilation = false;
    ImGui::PushID(0);
    if 
    (
        ImGui::SmallButton
        (
            gui_.isDetachedFromMenu ? 
            ICON_FA_WINDOW_MAXIMIZE : 
            ICON_FA_ARROW_RIGHT
        )
    )
        gui_.isDetachedFromMenu = !gui_.isDetachedFromMenu;
    ImGui::PopID();
    ImGui::SameLine();
    if (!gui_.isDetachedFromMenu)
    {
        if (ImGui::BeginMenu("Shared storage viewer"))
        {
            gui_.isOpen = true;
            shadersRequireRecompilation = renderGui();
            ImGui::EndMenu();
        }
        else
            gui_.isOpen = false;
        return shadersRequireRecompilation;
    }
    ImGui::MenuItem("Shared storage viewer", NULL, &gui_.isOpen);
    return shadersRequireRecompilation;
}

}