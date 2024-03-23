#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/objectio.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

SharedStorage::SharedStorage()
{
    buffer_ = vir::ShaderStorageBuffer::create(Block::size);
    if 
    (
        (
            vir::Window::instance()->context()->versionMajor() == 4 &&
            vir::Window::instance()->context()->versionMinor() >= 3
        ) ||
        vir::Window::instance()->context()->versionMajor() >= 4 // In the future
    )
        isSupported_ = true;
    if (isSupported_)
    {
        buffer_ = vir::ShaderStorageBuffer::create(Block::size);
        buffer_->bind();
        buffer_->setBindingPoint(bindingPoint_);
        block_.dataStart = buffer_->mapData();
        block_.ioIntData = (int*)block_.dataStart;
        block_.ioVec4Data = (glm::vec4*)(block_.ioIntData+Block::arraySize);
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
    DELETE_IF_NOT_NULLPTR(buffer_)
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

    sharedStorage->gui_ = gui;
    return sharedStorage;
}

//----------------------------------------------------------------------------//

void SharedStorage::clear()
{
    if (!isSupported_)
        return;
    buffer_->fenceSync();
    auto dataStart = (unsigned char*)block_.dataStart;
    for (int i=0; i<block_.size; i++)
        *(dataStart+i) = 0;
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
        shader->bindShaderStorageBlock(Block::glslName, bindingPoint_);
}

//----------------------------------------------------------------------------//

const char* SharedStorage::glslBlockSource() const
{
    if (isSupported_)
        return block_.glslSource;
    return "";
}

//----------------------------------------------------------------------------//

void SharedStorage::renderGui()
{
    if (!gui_.isOpen)
        return;
    const float fontSize = ImGui::GetFontSize();
    const float textWidth = 45.f*fontSize;
    if (gui_.isDetachedFromMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(600,300), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Shared storage viewer", &gui_.isOpen, windowFlags);

        // Refresh icon if needed
        static bool isIconSet(false);
        static bool isWindowDocked(ImGui::IsWindowDocked());
        if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
        {
            isIconSet = vir::ImGuiRenderer::setWindowIcon
            (
                "Shared storage viewer", 
                ByteData::Icon::sTIconData, 
                ByteData::Icon::sTIconSize,
                false
            );
            isWindowDocked = ImGui::IsWindowDocked();
        }
    }
    else
        ImGui::Dummy({textWidth, 0});

    /*ImGui::PushTextWrapPos
    (
        ImGui::GetCursorPos().x + 
        gui_.isDetachedFromMenu ? 
        ImGui::GetContentRegionAvail().x : 
        textWidth
    );
    ImGui::Text(
ICON_FA_EXCLAMATION_TRIANGLE " - While this panel is open, there is a minor "
"performance loss due to the need to sync GPU and CPU read/write operations, "
"which are required to show the values stored in these shared storage block "
"arrays"
    );
    ImGui::PopTextWrapPos();
    ImGui::Dummy
    (
        {
            gui_.isDetachedFromMenu ? 
            ImGui::GetContentRegionAvail().x : 
            textWidth, 
            0
        }
    );*/

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
                    std::min(gui_.ioIntDataViewStartIndex, Block::arraySize),
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
                    std::min(gui_.ioIntDataViewEndIndex, Block::arraySize),
                    0
                );
        ImGui::PopItemWidth();
        
        ImGui::EndChild();
    }
    ImGui::SameLine();
    
    
    {
        ImGui::BeginChild
        (
            "##sharedStorageVec4ControlsChild", 
            ImVec2(0, controlsHeight),
            false
        );
        ImGui::SeparatorText("ioVec4Data");

        ImGui::Text("View start        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", &gui_.ioVec4DataViewStartIndex))
            gui_.ioVec4DataViewStartIndex = 
                std::max
                (
                    std::min(gui_.ioVec4DataViewStartIndex, Block::arraySize),
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
                    std::min(gui_.ioVec4DataViewEndIndex, Block::arraySize),
                    0
                );
        ImGui::PopItemWidth();

        ImGui::Text("Show color        ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##ioVec4DataShowColor", 
            &gui_.isVec4DataAlsoShownAsColor
        );

        ImGui::Text("Precision, format ");
        ImGui::SameLine();
        ImGui::PushItemWidth(10*fontSize);
        if 
        (
            ImGui::SliderInt
            (
                "##ioVec4DataPrecision", 
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
        static const char* labels[4] = {"x, ", "y, ", "z, ", "w"};
        for (int i=0; i<4; i++)
        {
            ImGui::SameLine();
            std::string label = "##ioVec4DataShowCmpt"+std::to_string(i);
            ImGui::Checkbox(label.c_str(), &gui_.ioVec4DataViewComponents[i]);
            ImGui::SameLine();
            ImGui::Text(labels[i]);
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
                ImGui::Text("%d", block_.ioIntData[row]);
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
            "##sharedStorageVec4RangeViewerChild", 
            ImVec2(0, rangeViewerHeight),
            false
        );
        if 
        (
            ImGui::BeginTable
            (
                "##sharedStorageVec4DataTable", 
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
                const glm::vec4& value = block_.ioVec4Data[row];
                if (gui_.isVec4DataAlsoShownAsColor)
                {
                    glm::vec4 valueCopy(value);
                    ImGui::ColorEdit4
                    (
                        "##ioVec4DataColorPicker", 
                        &(valueCopy.x),
                        ImGuiColorEditFlags_NoInputs
                    );
                    ImGui::SameLine();
                }
                for (int i=0; i<4; i++)
                {
                    if (!gui_.ioVec4DataViewComponents[i])
                        continue;
                    ImGui::Text
                    (
                        gui_.ioVec4DataViewFormat.c_str(),
                        value[i]
                    );
                    if (i < 3)
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
    ImGui::MenuItem("Shared storage viewer", NULL, & gui_.isOpen);
}

}