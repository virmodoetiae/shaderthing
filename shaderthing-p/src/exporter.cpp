#include "shaderthing-p/include/exporter.h"
#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/macros.h"
#include "vir/include/vir.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

Exporter::Exporter()
{
    tuneIntoEventBroadcaster();
    gifEncoder_ = new vir::GifEncoder();
    auto window = vir::GlobalPtr<vir::Window>::instance();
    settings_.outputResolution = {window->width(), window->height()};
}

//----------------------------------------------------------------------------//

Exporter::~Exporter()
{
    DELETE_IF_NOT_NULLPTR(gifEncoder_);
}

//----------------------------------------------------------------------------//

void Exporter::onReceive(vir::Event::WindowResizeEvent& event)
{
    settings_.outputResolution = {event.width, event.height};
    settings_.outputResolutionChanged = true;
}

//----------------------------------------------------------------------------//

void Exporter::update()
{

}

//----------------------------------------------------------------------------//

void Exporter::renderGUI(const std::vector<Layer*>& layers)
{
    float fontSize(ImGui::GetFontSize());
    float x0 = ImGui::GetCursorPosX();
    ImGui::Text("Export type                 ");
    ImGui::SameLine();

    static std::map<ExportType, const char*> exportTypeToName = {
        {ExportType::Image, "Image"},
        {ExportType::GIF, "GIF"},
        {ExportType::VideoFrames, "Video frames"}
    };

    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::BeginCombo
        (
            "##exportTypeSelector", 
            exportTypeToName[exportType_]
        )
    )
    {
        for(auto e : exportTypeToName)
        {
            bool disabled
            (
                e.first == ExportType::GIF && 
                !gifEncoder_->canRunOnDeviceInUse()
            );
            if (disabled)
                ImGui::BeginDisabled();
            if 
            (
                ImGui::Selectable
                (
                    disabled ? 
                    "GIF (requires OpenGL v4.3 or above)" :
                    e.second
                )
            )
                exportType_ = e.first;
            if (disabled)
                ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (exportType_ != ExportType::Image)
    {
        ImGui::Text("Start time                  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat("##exporterStartTime", &settings_.startTime);
        ImGui::PopItemWidth();
        ImGui::Text("End time                    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat("##exporterEndTime", &settings_.startTime);
        ImGui::PopItemWidth();
        ImGui::Text("Target FPS                  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        float fps0(settings_.fps);
        ImGui::InputFloat
        (
            "##exporterFps", 
            &settings_.fps, 
            0.0f, 
            0.0f, 
            "%.2f"
        );
        ImGui::PopItemWidth();
        if (exportType_ == ExportType::GIF)
        {
            if (fps0 != settings_.fps)
                fps0 = 100.0f/int(100.0f/settings_.fps+.5f);
            
            ImGui::Text("Palette bit depth           ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(Determines the GIF palette size. The number of colors in the palette is 
2^(palette bit depth))");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            ImGui::SliderInt
            (
                "##exporterGifPaletteBitDepth", 
                (int*)&settings_.gifPaletteBitDepth, 
                2, 
                8
            );
            ImGui::PopItemWidth();

            ImGui::Text("Update palette every frame  ");
            ImGui::SameLine();
            ImGui::Checkbox
            (
                "##exporterIsGifPaletteDynamic", 
                &settings_.isGifPaletteDynamic
            );
            
            ImGui::Text("Transparency cutoff         ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(Alpha channel threshold below which (threshold excluded) a pixel is considered
as fully transparent. If set to 0, the output GIF file is fully opaque with no
transparency)");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            ImGui::SliderInt
            (
                "##exportToolAlphaCutoff", 
                (int*)&settings_.gifAlphaCutoff, 
                0, 
                255
            );
            ImGui::PopItemWidth();
            ImGui::Text("Color dithering             ");
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::BeginCombo
                (
                    "##gifDitheringSelector", 
                    vir::Quantizer::Settings::ditherModeToName.at
                    (
                        settings_.gifDitherMode
                    ).c_str()
                )
            )
            {
                for(auto e : vir::Quantizer::Settings::ditherModeToName)
                {
                    if (ImGui::Selectable(e.second.c_str()))
                    {
                        settings_.gifDitherMode = e.first;
                        break;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
    }
    if (exportType_ != ExportType::Image)
        ImGui::Text("Render passes per frame     ");
    else
        ImGui::Text("Render passes               ");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    ImGui::InputInt("##nRenderPasses", (int*)&settings_.nRenderPasses);
    settings_.nRenderPasses = 
        std::min(std::max(1u, settings_.nRenderPasses), 1000u);
    ImGui::PopItemWidth();
    if (settings_.nRenderPasses > 1 && exportType_ != ExportType::Image)
    {
        ImGui::Text("Multi-pass first frame only ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##exportAreRenderPassesOnFirstFrameOnly", 
            &settings_.areRenderPassesOnFirstFrameOnly
        );
    }

    //--------------------------------------------------------------------------

    auto window = vir::GlobalPtr<vir::Window>::instance();

    ImGui::Separator();
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    int deleteRow = -1;
    if (ImGui::BeginTable("##exportResolutionTable", 3, flags))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("Layer name", flags, 10.0f*fontSize);
        ImGui::TableSetupColumn("Export resolution", flags, 10.0f*fontSize);
        float width = 2.f*fontSize;
        ImGui::TableSetupColumn("", flags, width);
        ImGui::TableHeadersRow();

        glm::ivec2& outputResolution = settings_.outputResolution;
        bool& outputResolutionChanged = settings_.outputResolutionChanged;
        float& outputScale = settings_.outputResolutionScale;
        
        // Actual table render -------------------------------------------------
        int nRows = (int)layers.size();
        for (int row = 0; row < nRows+1; row++)
        {
            int col = 0;
            
            ImGui::PushID(row);
            ImGui::TableNextRow(0, 1.6*fontSize);
            
            if (row == 0) //
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::Text("Output image");
                
                ImGui::TableSetColumnIndex(col++);
                ImGui::PushItemWidth(-1);
                glm::ivec2 outputResolution0 = outputResolution;
                ImGui::InputInt2
                (
                    "##exportResolution", 
                    glm::value_ptr(outputResolution)
                );
                if (outputResolution0.x != outputResolution.x)
                {
                    outputResolution.y = 
                        std::max(outputResolution.x/window->aspectRatio(), 1.f);
                    outputResolutionChanged = true;
                }
                else if (outputResolution0.y != outputResolution.y)
                {
                    outputResolution.x = 
                        std::max(outputResolution.y*window->aspectRatio(), 1.f);
                    outputResolutionChanged = true;
                }
                outputResolution.x=std::max(outputResolution.x, 1);
                outputResolution.y=std::max(outputResolution.y, 1);
                if (outputResolutionChanged)
                {
                    outputScale = 
                        outputResolution.x > outputResolution.y ?
                        (float)outputResolution.x/window->width() :
                        (float)outputResolution.y/window->height();
                }
                ImGui::PopItemWidth();
                ImGui::TableSetColumnIndex(col++);
                ImGui::PopID();
                continue;
            }
            
            auto layer = layers[row-1];
            bool layerRendersToWindow
            (
                layer->renderingTarget() == Layer::Rendering::Target::Window
            );
            auto& exportData = layer->exportData();
            auto& layerExportResolution = exportData.resolution;
            auto& layerExportScale = exportData.resolutionScale;
            auto& layerLocked = exportData.resolutionLocked;

            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            ImGui::Text(layer->name().c_str());
            ImGui::PopItemWidth();

            // Column 1 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (outputResolutionChanged)
            {
                exportData.windowResolutionScale = outputScale;
                if (layerLocked)
                    layerExportResolution = 
                        (glm::vec2)layer->resolution()*
                        layerExportScale*outputScale + .5f;
            }
            else if (!layerRendersToWindow)
            {
                glm::ivec2 layerExportResolution0 = layerExportResolution;
                bool layerExportResolutionChanged = 
                    ImGui::InputInt2
                    (
                        "##exporterLayerResolution", 
                        glm::value_ptr(layerExportResolution)
                    );
                if (layerExportResolution0.x != layerExportResolution.x)
                {
                    layerExportResolution.y = 
                        std::max
                        (
                            layerExportResolution.x/layer->aspectRatio(),
                            1.f
                        );
                }
                else if (layerExportResolution0.y != layerExportResolution.y)
                    layerExportResolution.x = 
                        std::max
                        (
                            layerExportResolution.y*layer->aspectRatio(),
                            1.f
                        );
                if (layerExportResolutionChanged)
                {
                    layerExportScale =
                        layerExportResolution.x > layerExportResolution.y ?
                        (float)layerExportResolution.x/layer->resolution().x :
                        (float)layerExportResolution.y/layer->resolution().y;
                }
            }
            else
            {
                layerExportResolution = outputResolution;
                ImGui::BeginDisabled();
                ImGui::InputInt2
                (
                    "##exporterResolution", 
                    glm::value_ptr(layerExportResolution)
                );
                ImGui::EndDisabled();
            }
            ImGui::PopItemWidth();

            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            if (layerRendersToWindow)
            {
                ImGui::PopID();
                continue;
            }

            ImGui::PushItemWidth(-1);
            bool layerLocked0(layerLocked);
            if 
            (
                ImGui::Button
                (
                    layerLocked ? 
                    ICON_FA_LOCK : 
                    ICON_FA_LOCK_OPEN, 
                    {-1,0}
                )
            )
                layerLocked = !layerLocked;
            /*
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(When the lock is active, the export resolution of this layer will not be 
modified upon changes to the output export resolution. Nonetheless, the
export resolution of this layer may still be modified, although the aspect 
ratio is always locked to the aspect ratio of the source layer)"
                );
                ImGui::EndTooltip();
            }*/
            if (layerLocked && !layerLocked0)
            {
                layerExportScale = 1;
                layerExportResolution = 
                    (glm::vec2)layer->resolution()*
                    layerExportScale*outputScale + .5f;
            }
            ImGui::PopItemWidth();
            
            ImGui::PopID();

            layerExportResolution.x=std::max(layerExportResolution.x, 1);
            layerExportResolution.y=std::max(layerExportResolution.y, 1);
        }
        ImGui::EndTable();
    }
    settings_.outputResolutionChanged = false;

    //--------------------------------------------------------------------------

    ImGui::Separator();
    exportButtonGUI();
    if (isRunning_)
        ImGui::ProgressBar
        (
            (float)frame_/std::max(nFrames_, 1u), 
            ImVec2(-1, 0.0f)
        );
}

//----------------------------------------------------------------------------//

void Exporter::exportButtonGUI()
{
    if (isRunning_)
        ImGui::BeginDisabled();
    if (ImGui::Button(isRunning_ ? "Exporting..." : "Export", {-1,0}))
    {
        std::vector<std::string> filters;
        if (exportType_ == ExportType::GIF)
            filters = {"Image files (*.gif)", "*.gif"};
        else
            filters = {"Image files (*.png)", "*.png"};
        fileDialog_.runSaveFileDialog("Export graphics to file", filters);
    }
    if (isRunning_)
        ImGui::EndDisabled();
}

}