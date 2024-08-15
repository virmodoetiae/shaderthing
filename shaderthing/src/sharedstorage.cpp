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
    const unsigned int nFloatComponents
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
    auto typedBlock = new TypedBlock<I, F, N>();                            \
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
    if 
    (
        (
            vir::Window::instance()->context()->versionMajor() == 4 &&
            vir::Window::instance()->context()->versionMinor() >= 3
        ) ||
        vir::Window::instance()->context()->versionMajor() > 4 // In the future
    )
        isSupported_ = true;
    if (isSupported_)
    {
        resetBlockAndSSBO(Block::IntType::I32, Block::FloatType::F64);
        gui_.ioVec4DataViewFormat =
        (
            "%."+
            std::to_string(gui_.ioVec4DataViewPrecision)+
            (gui_.ioVec4DataViewExponentialFormat ? "e" : "f")
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
        io.write(TO_STRING(Name), gui_.Name);\

    WRITE_GUI_ITEM(ioIntDataViewStartIndex)
    WRITE_GUI_ITEM(ioIntDataViewEndIndex)
    WRITE_GUI_ITEM(ioVec4DataViewEndIndex)
    WRITE_GUI_ITEM(ioVec4DataViewStartIndex)
    WRITE_GUI_ITEM(isVec4DataAlsoShownAsColor)
    WRITE_GUI_ITEM(ioVec4DataViewPrecision)
    WRITE_GUI_ITEM(ioVec4DataViewExponentialFormat)
    glm::ivec4 values(4);
    for (int i=0; i<4; i++)
        values[i] = int(gui_.ioVec4DataViewComponents[i]);
    io.write("ioVec4DataViewComponents", values);

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

#define READ_GUI_ITEM(Name, Type)                                  \
    gui.Name = ioSS.readOrDefault<Type>(TO_STRING(Name),gui.Name);

    READ_GUI_ITEM(ioIntDataViewStartIndex, int)
    READ_GUI_ITEM(ioIntDataViewEndIndex, int)
    READ_GUI_ITEM(ioVec4DataViewEndIndex, int)
    READ_GUI_ITEM(ioVec4DataViewStartIndex, int)
    READ_GUI_ITEM(isVec4DataAlsoShownAsColor, bool)
    READ_GUI_ITEM(ioVec4DataViewPrecision, int)
    READ_GUI_ITEM(ioVec4DataViewExponentialFormat, bool)
    glm::ivec4 values = 
        ioSS.readOrDefault<glm::ivec4>
        (
            "ioVec4DataViewComponents", 
            glm::ivec4{1, 1, 1, 1}
        );
    for (int i=0; i<4; i++)
        gui.ioVec4DataViewComponents[i] = bool(values[i]);
    gui.ioVec4DataViewFormat =
    (
        "%."+
        std::to_string(gui.ioVec4DataViewPrecision)+
        (gui.ioVec4DataViewExponentialFormat ? "e" : "f")
    );
    sharedStorage->gui_ = gui;
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

void SharedStorage::renderGui()
{
    if (!gui_.isOpen)
        return;
    if (!isSupported_)
    {
        ImGui::Text
        (
            "Not supported due to OpenGL version <= 4.2, requires >= 4.3"
        );
        return;
    }
    const float fontSize = ImGui::GetFontSize();
    const float textWidth = 45.f*fontSize;
    if (gui_.isDetachedFromMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(600,300), ImGuiCond_FirstUseEver);
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

    float controlsHeight = 8*ImGui::GetTextLineHeightWithSpacing();
    {
        ImGui::BeginChild
        (
            "##sharedStorageIntControlsChild", 
            ImVec2(ImGui::GetContentRegionAvail().x * 0.30f, controlsHeight),
            false
        );
        ImGui::SeparatorText("ioIntData");
        
        ImGui::Text("View start ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", &gui_.ioIntDataViewStartIndex))
            gui_.ioIntDataViewStartIndex = 
                std::max
                (
                    std::min
                    (
                        gui_.ioIntDataViewStartIndex, 
                        SHARED_STORAGE_INT_ARRAY_SIZE
                    ),
                    0
                );
        ImGui::PopItemWidth();
        
        ImGui::Text("View end   ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##intEndIndex", &gui_.ioIntDataViewEndIndex);
            gui_.ioIntDataViewEndIndex = 
                std::max
                (
                    std::min
                    (
                        gui_.ioIntDataViewEndIndex, 
                        SHARED_STORAGE_INT_ARRAY_SIZE
                    ),
                    0
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
        ImGui::SeparatorText("ioVecData");

        ImGui::Text("View start        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", &gui_.ioVec4DataViewStartIndex))
            gui_.ioVec4DataViewStartIndex = 
                std::max
                (
                    std::min(gui_.ioVec4DataViewStartIndex, SHARED_STORAGE_VEC4_ARRAY_SIZE),
                    0
                );
        ImGui::PopItemWidth();
        
        ImGui::Text("View end          ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##intEndIndex", &gui_.ioVec4DataViewEndIndex);
            gui_.ioVec4DataViewEndIndex = 
                std::max
                (
                    std::min(gui_.ioVec4DataViewEndIndex, SHARED_STORAGE_VEC4_ARRAY_SIZE),
                    0
                );
        ImGui::PopItemWidth();

        ImGui::Text("Show color        ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##ioVecDataShowColor", 
            &gui_.isVec4DataAlsoShownAsColor
        );

        ImGui::Text("Precision, format ");
        ImGui::SameLine();
        ImGui::PushItemWidth(10*fontSize);
        if 
        (
            ImGui::SliderInt
            (
                "##ioVecDataPrecision", 
                &gui_.ioVec4DataViewPrecision, 
                0, 
                15
            )
        )
        {
            gui_.ioVec4DataViewFormat = 
                "%."+
                std::to_string(gui_.ioVec4DataViewPrecision)+
                (gui_.ioVec4DataViewExponentialFormat ? "e" : "f");
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                gui_.ioVec4DataViewExponentialFormat ?
                "Scientific" : "Decimal",
                {-1, 0}
            )
        )
        {
            gui_.ioVec4DataViewExponentialFormat = 
                !gui_.ioVec4DataViewExponentialFormat;
            gui_.ioVec4DataViewFormat = 
                "%."+
                std::to_string(gui_.ioVec4DataViewPrecision)+
                (gui_.ioVec4DataViewExponentialFormat ? "e" : "f");
        }
        ImGui::Text("Show components   ");
        static const char* labels[4] = {"x ", "y ", "z ", "w"};
        for (int i=0; i<block_->nFloatComponents(); i++)
        {
            ImGui::SameLine();
            std::string label = "##ioVecDataShowCmpt"+std::to_string(i);
            ImGui::Checkbox(label.c_str(), &gui_.ioVec4DataViewComponents[i]);
            ImGui::SameLine();
            ImGui::Text("%s", labels[i]);
        }

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
                int row = gui_.ioIntDataViewStartIndex;
                row <= gui_.ioIntDataViewEndIndex;
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
                int row = gui_.ioVec4DataViewStartIndex;
                row <= gui_.ioVec4DataViewEndIndex;
                row++
            )
            {
                ImGui::TableNextRow();
                ImGui::PushID(row);
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", row);
                ImGui::TableSetColumnIndex(1);
                /*
                const auto& value = block_.ioVec4Data[row];
                if (gui_.isVec4DataAlsoShownAsColor)
                {
                    // Has to be (possibly) downcasted to a float to work with 
                    // ColorEdit4
                    glm::vec4 valueCopy(value);
                    ImGui::ColorEdit4
                    (
                        "##ioVec4DataColorPicker", 
                        &(valueCopy.x),
                        ImGuiColorEditFlags_NoInputs
                    );
                    ImGui::SameLine();
                }*/
                for (int i=0; i<block_->nFloatComponents(); i++)
                {
                    if (!gui_.ioVec4DataViewComponents[i])
                        continue;
                    block_->printVec4(&ImGui::Text, row, i, gui_.ioVec4DataViewFormat.c_str());
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
}

//----------------------------------------------------------------------------//

void SharedStorage::renderMenuItemGui()
{
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
            renderGui();
            ImGui::EndMenu();
        }
        else
            gui_.isOpen = false;
        return;
    }
    ImGui::MenuItem("Shared storage viewer", NULL, &gui_.isOpen);
}

}