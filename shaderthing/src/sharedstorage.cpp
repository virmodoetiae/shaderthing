#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/macros.h"

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
    }
}

SharedStorage::~SharedStorage()
{
    DELETE_IF_NOT_NULLPTR(buffer_)
}

void SharedStorage::clear()
{
    if (!isSupported_)
        return;
    buffer_->fenceSync();
    auto dataStart = (unsigned char*)block_.dataStart;
    for (int i=0; i<block_.size; i++)
        *(dataStart+i) = 0;
}

void SharedStorage::gpuMemoryBarrier() const
{
    if (isSupported_)
        buffer_->memoryBarrier();
}

void SharedStorage::cpuMemoryBarrier() const
{
    if (isSupported_)
        buffer_->fenceSync();
}

void SharedStorage::bindShader(vir::Shader* shader)
{
    if (isSupported_)
        shader->bindShaderStorageBlock(Block::glslName, bindingPoint_);
}

const char* SharedStorage::glslBlockSource() const
{
    if (isSupported_)
        return block_.glslSource;
    return "";
}

void SharedStorage::renderGui()
{
    if (!gui_.isOpen)
        return;
    const float fontSize = ImGui::GetFontSize();
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
        ImGui::Dummy({40.f*fontSize, 0});

    float controlsHeight = 4*ImGui::GetTextLineHeightWithSpacing();

    {
        ImGui::BeginChild
        (
            "##sharedStorageIntControlsChild", 
            ImVec2(ImGui::GetContentRegionAvail().x * 0.34f, controlsHeight),
            false
        );
        ImGui::SeparatorText("ioIntData");
        
        ImGui::Text("View start ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", &gui_.ioIntDataViewStartIndex))
            gui_.ioIntDataViewStartIndex = 
                std::max(std::min(gui_.ioIntDataViewStartIndex, Block::arraySize), 0);
        ImGui::PopItemWidth();
        
        ImGui::Text("View end   ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##intEndIndex", &gui_.ioIntDataViewEndIndex);
            gui_.ioIntDataViewEndIndex = 
                std::max(std::min(gui_.ioIntDataViewEndIndex, Block::arraySize), 0);
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

        ImGui::Text("View start ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##intStartIndex", &gui_.ioVec4DataViewStartIndex))
            gui_.ioVec4DataViewStartIndex = 
                std::max(std::min(gui_.ioVec4DataViewStartIndex, Block::arraySize), 0);
        ImGui::PopItemWidth();
        
        ImGui::Text("View end   ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##intEndIndex", &gui_.ioVec4DataViewEndIndex);
            gui_.ioVec4DataViewEndIndex = 
                std::max(std::min(gui_.ioVec4DataViewEndIndex, Block::arraySize), 0);
        ImGui::PopItemWidth();

        ImGui::EndChild();
    }

    ImGui::Separator();

    buffer_->fenceSync(); // Ensure all shaders have finished writing data

    float rangeViewerHeight = 
        gui_.isDetachedFromMenu ? 
        ImGui::GetContentRegionAvail().y :
        9*ImGui::GetTextLineHeightWithSpacing();
    float indexColumnWidth = 0;

    {
        ImGui::BeginChild
        (
            "##sharedStorageIntRangeViewerChild", 
            ImVec2(ImGui::GetContentRegionAvail().x * 0.34f, rangeViewerHeight),
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
            indexColumnWidth = ImGui::GetContentRegionAvail().x/4;
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
                std::string format = "%.3f, %.3f, %.3f, %.3f";
                for (int i=0; i<4; i++)
                {
                    float v(std::abs(value[i]));
                    if ((v < 1e-3 || v > 1e3) && v != 0)
                        format[3+5*i] = 'e';
                }
                ImGui::Text
                (
                    format.c_str(), 
                    value.x,
                    value.y,
                    value.z,
                    value.w
                );
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

void SharedStorage::renderMenuItemGui()
{
    ImGui::PushID(0);
    if (ImGui::SmallButton(gui_.isDetachedFromMenu ? "Z" : "O" ))
    {
        gui_.isDetachedFromMenu = !gui_.isDetachedFromMenu;
    }
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