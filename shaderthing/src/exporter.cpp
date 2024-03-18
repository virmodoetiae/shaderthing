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

#include <charconv>

#include "shaderthing/include/exporter.h"

#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/macros.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/shareduniforms.h"

#include "vir/include/vir.h"

#include "thirdparty/stb/stb_image_write.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

Exporter::Exporter()
{
    tuneIntoEventBroadcaster();
    gifEncoder_ = new vir::GifEncoder();
}

//----------------------------------------------------------------------------//

Exporter::~Exporter()
{
    DELETE_IF_NOT_NULLPTR(gifEncoder_);
}

//----------------------------------------------------------------------------//

void Exporter::onReceive(vir::Event::WindowResizeEvent& event)
{
    settings_.outputResolutionChanged = true;
}

//----------------------------------------------------------------------------//

void Exporter::update
(
    SharedUniforms& sharedUniforms,
    const std::vector<Layer*>& layers,
    const UpdateArgs& updateArgs
)
{
    if (!isRunning_)
    {
        if (fileDialog_.validSelection())
        {
            isRunning_ = true;
            settings_.outputFilepath = fileDialog_.selection().front();
            fileDialog_.clearSelection();
        }
        else
            return;
        
        // Setup
        frame_ = 0;
        if (exportType_ != ExportType::Image)
        {
            float Dt = settings_.endTime-settings_.startTime;
            nFrames_ = std::max(settings_.fps*Dt+.5f, 1.f);
            timeStep_ = Dt/nFrames_;
        }
        else
        {
            nFrames_ = 1;
            timeStep_ = 0.f;
        }
        updateArgs.timeStep = 0.f;

        DELETE_IF_NOT_NULLPTR(framebuffer_);
        auto outputResolution = sharedUniforms.exportData().resolution;
        framebuffer_ = vir::Framebuffer::create
        (
            outputResolution.x, 
            outputResolution.y
        );
        DELETE_IF_NOT_NULLPTR(framebufferData_);
        framebufferData_ = 
            new unsigned char[framebuffer_->colorBufferDataSize()];

        sharedUniforms.prepareForExport(settings_.startTime);
        for (auto layer : layers)
            layer->prepareForExport();

        if (exportType_ == ExportType::GIF && !gifEncoder_->isFileOpen())
            gifEncoder_->openFile
            ( 
                settings_.outputFilepath.c_str(),
                sharedUniforms.exportData().resolution.x, 
                sharedUniforms.exportData().resolution.y, 
                settings_.gifPaletteBitDepth,
                settings_.gifAlphaCutoff > 0 ?
                    vir::Quantizer::Settings::IndexMode::Alpha :
                    vir::Quantizer::Settings::IndexMode::Default
            );

        return;
    }
    else // Save to disk
    {
        switch (exportType_)
        {
            case (ExportType::Image) :
            case (ExportType::VideoFrames) :
            {
                const char* filepath;
                if (exportType_ == ExportType::VideoFrames)
                {
                    // Save frame FPS and frame number in out path as well. If
                    // the FPS is not an integer, the '.' is replaced by a 'd'
                    // (for 'dot') so not to mess with the file extension
                    auto fps = Helpers::format(settings_.fps, 2);
                    std::replace(fps.begin(), fps.end(), '.', 'd');
                    fps = "_"+fps+"_"+std::to_string(frame_);
                    cache_.outputFilepathExtended = 
                        Helpers::appendToFilename
                        (
                            settings_.outputFilepath, 
                            fps
                        );
                    filepath = cache_.outputFilepathExtended.c_str();
                }
                else
                    filepath = settings_.outputFilepath.c_str();
                framebuffer_->colorBufferData(framebufferData_, true);
                stbi_write_png
                (
                    filepath, 
                    sharedUniforms.exportData().resolution.x,
                    sharedUniforms.exportData().resolution.y,
                    4, 
                    (const void*)framebufferData_, 
                    sharedUniforms.exportData().resolution.x*4
                );
                break;
            }
            case (ExportType::GIF) :
            {
                gifEncoder_->encodeFrame
                (
                    framebuffer_,
                    {   // Encoding options
                        std::max(int(100.0f/settings_.fps), 1), // fps
                        true,                                   // flip Y
                        settings_.gifDitherMode,                // 
                        0,                                      // Dither thres.
                        (int)settings_.gifAlphaCutoff,          //
                        settings_.isGifPaletteDynamic           //
                    }
                );
                break;
            }
        }

        if (frame_ == nFrames_-1) // Terminate
        {
            isRunning_ = false;
            frame_ = 0;
            
            DELETE_IF_NOT_NULLPTR(framebuffer_)
            DELETE_IF_NOT_NULLPTR(framebufferData_)

            if (gifEncoder_->isFileOpen())
                gifEncoder_->closeFile();

            sharedUniforms.resetAfterExport
            (
                settings_.resetFrameCounterAfterExport
            );
            for (auto layer : layers)
                layer->resetAfterExport();
        }
        else
            ++frame_;
        updateArgs.timeStep = timeStep_;
    }
}

//----------------------------------------------------------------------------//

void Exporter::renderGui
(
    SharedUniforms& sharedUniforms,
    const std::vector<Layer*>& layers
)
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
            {
                exportType_ = e.first;
                // Ensure valid GIF fps (can only be such that the resulting
                // frame duration is an integer number of hundreds of a second)
                if (exportType_ == ExportType::GIF)
                    settings_.fps = 
                        100.f/int(100.f/std::min(settings_.fps,100.f)+.5f);
            }
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
        ImGui::InputFloat("##exporterEndTime", &settings_.endTime);
        ImGui::PopItemWidth();
        ImGui::Text("Target FPS                  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        bool fpsChanged = ImGui::InputFloat
        (
            "##exporterFps", 
            &settings_.fps, 
            0.0f, 
            0.0f, 
            "%.2f"
        );
        ImGui::PopItemWidth();
        
        ImGui::PushItemWidth(-1);
        if (exportType_ == ExportType::GIF)
        {
            // Ensure valid GIF fps (can only be such that the resulting frame
            // duration is an integer number of hundreds of a second)
            if (fpsChanged)
                settings_.fps = 
                    100.f/int(100.f/std::min(settings_.fps,100.f)+.5f);
            
            ImGui::Text("Palette bit depth           ");
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
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
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                ImGui::BeginTooltip()
            )
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

    ImGui::Text("Reset iFrame after export   ");
    ImGui::SameLine();
    ImGui::Checkbox
    (
        "##resetFrameCounterAfterExport", 
        &settings_.resetFrameCounterAfterExport
    );

    //--------------------------------------------------------------------------

    auto window = vir::Window::instance();

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
        ImGui::TableSetupColumn("Layer name", flags, 9.f*fontSize);
        ImGui::TableSetupColumn("Export resolution", flags, 9.25f*fontSize);
        ImGui::TableSetupColumn("Clear policy", flags, 9.5f*fontSize);
        ImGui::TableHeadersRow();

        glm::ivec2& outputResolution = sharedUniforms.exportData().resolution;
        float& outputScale = sharedUniforms.exportData().resolutionScale;
        bool& outputResolutionChanged = settings_.outputResolutionChanged;
        
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
                ImGui::PushItemWidth(6.75*fontSize);
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
            auto& layerRescale = exportData.rescaleWithOutput;

            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            ImGui::Text(layer->name().c_str());
            ImGui::PopItemWidth();

            // Column 1 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(6.75*fontSize);
            if (outputResolutionChanged)
            {
                exportData.windowResolutionScale = outputScale;
                if (layerRescale)
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
                        (float)layerExportResolution.x/outputScale/layer->resolution().x :
                        (float)layerExportResolution.y/outputScale/layer->resolution().y;
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
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            bool layerRescale0(layerRescale);
            if (layerRendersToWindow)
                ImGui::BeginDisabled();
            if 
            (
                ImGui::Button
                (
                    layerRescale ? 
                    ICON_FA_LOCK : 
                    ICON_FA_LOCK_OPEN, 
                    {-1,0}
                )
            )
                layerRescale = !layerRescale;
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
                ImGui::BeginTooltip()
            )
            {
                if (layerRescale)
                    ImGui::Text(
ICON_FA_LOCK " - The layer resolution will rescale\nwith the output resolution"
                    );
                else
                    ImGui::Text(
ICON_FA_LOCK_OPEN " - The layer resolution will not rescale\nwith the output "
"resolution"
                    );
                ImGui::EndTooltip();
            }
            if (layerRendersToWindow)
                ImGui::EndDisabled();
            if (layerRescale && !layerRescale0)
            {
                layerExportScale = 1;
                layerExportResolution = 
                    (glm::vec2)layer->resolution()*
                    layerExportScale*outputScale + .5f;
            }
            ImGui::PopItemWidth();

            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            if (!layerRendersToWindow)
            {
                static std::map
                <
                    Layer::ExportData::FramebufferClearPolicy, 
                    std::string
                > frameBufferClearPolicyToName
                {
                    {
                        Layer::ExportData::FramebufferClearPolicy::
                        ClearOnEveryFrameExport, 
                        "On every frame"
                    },
                    {
                        Layer::ExportData::FramebufferClearPolicy::
                        ClearOnFirstFrameExport, 
                        "On first frame"
                    },
                    {Layer::ExportData::FramebufferClearPolicy::None, "None"}
                };
                auto selectedExportClearPolicy = 
                        frameBufferClearPolicyToName.at(exportData.clearPolicy);
                ImGui::PushItemWidth(-1);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##layerExportClearPolicy",
                        selectedExportClearPolicy.c_str()
                    )
                )
                {
                    for (auto entry : frameBufferClearPolicyToName)
                    {
                        if (ImGui::Selectable(entry.second.c_str()))
                            exportData.clearPolicy = entry.first;
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }
            
            ImGui::PopID();

            layerExportResolution.x=std::max(layerExportResolution.x, 1);
            layerExportResolution.y=std::max(layerExportResolution.y, 1);
        }
        ImGui::EndTable();
    }
    settings_.outputResolutionChanged = false;

    //--------------------------------------------------------------------------

    ImGui::Separator();
    exportButtonGui();
    if (isRunning_)
        ImGui::ProgressBar
        (
            (float)frame_/std::max(nFrames_, 1u), 
            ImVec2(-1, 0.0f)
        );
}

//----------------------------------------------------------------------------//

void Exporter::exportButtonGui()
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

//----------------------------------------------------------------------------//

void Exporter::save(ObjectIO& io)
{
    io.writeObjectStart("exporter");
    io.write("type", (int)exportType_);
    io.write("outputFilepath", settings_.outputFilepath);
    io.write("nRenderPasses", settings_.nRenderPasses);
    io.write("areRenderPassesOnFirstFrameOnly", settings_.areRenderPassesOnFirstFrameOnly);
    io.write("resetFrameCounterAfterExport", settings_.resetFrameCounterAfterExport);
    io.write("startTime", settings_.startTime);
    io.write("endTime", settings_.endTime);
    io.write("fps", settings_.fps);
    io.write("isGifPaletteDynamic", settings_.isGifPaletteDynamic);
    io.write("gifPaletteBitDepth", settings_.gifPaletteBitDepth);
    io.write("gifAlphaCutoff", settings_.gifAlphaCutoff);
    io.write("gifDitherMode", (int)settings_.gifDitherMode);
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void Exporter::load(const ObjectIO& io, Exporter*& exporter)
{
    DELETE_IF_NOT_NULLPTR(exporter)
    exporter = new Exporter();
    if (!io.hasMember("exporter"))
        return;
    
    auto ioEx = io.readObject("exporter");
    exporter->exportType_ = (ExportType)ioEx.read<int>("type");
    auto settings = Settings{};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define READ_SETTINGS_ITEM(Name, Type)                                      \
    settings.Name = ioEx.readOrDefault<Type>(TOSTRING(Name),settings.Name);
#define READ_SETTINGS_ITEM2(Name, Type, Type2)                              \
    settings.Name = (Type2)(ioEx.readOrDefault<Type>(TOSTRING(Name),        \
                                              (Type)settings.Name));
    
    READ_SETTINGS_ITEM(outputFilepath, std::string)
    READ_SETTINGS_ITEM(nRenderPasses, int)
    READ_SETTINGS_ITEM(areRenderPassesOnFirstFrameOnly, bool)
    READ_SETTINGS_ITEM(resetFrameCounterAfterExport, bool)
    READ_SETTINGS_ITEM(startTime, float)
    READ_SETTINGS_ITEM(endTime, float)
    READ_SETTINGS_ITEM(fps, float)
    READ_SETTINGS_ITEM(isGifPaletteDynamic, bool)
    READ_SETTINGS_ITEM(gifPaletteBitDepth, int)
    READ_SETTINGS_ITEM(gifAlphaCutoff, int)
    READ_SETTINGS_ITEM2(gifDitherMode, int, DitherMode)

    exporter->settings_ = settings;
}

}