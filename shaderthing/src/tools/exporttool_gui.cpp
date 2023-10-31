/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthingapp.h"
#include "layers/layer.h"
#include "tools/exporttool.h"
#include "data/data.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/stb/stb_image_write.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

void ExportTool::renderGui()
{
    if (!isGuiOpen_)
        return;

    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,350), ImGuiCond_FirstUseEver);
        //ImGui::SetNextWindowBgAlpha(0.25f);
        static ImGuiWindowFlags windowFlags
        (
            ImGuiWindowFlags_NoCollapse
        );
        ImGui::Begin("Export manager", &isGuiOpen_, windowFlags);
        static bool setIcon(false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "Export manager", 
                IconData::sTIconData, 
                IconData::sTIconSize,
                false
            );
        }
    }
    
    float fontSize(ImGui::GetFontSize());
    float entryWidth = -1;

    //--------------------------------------------------------------------------

    // Unorthodox way to get and store any errors
    static bool firstCall(true);
    static bool isGifSupported(true);
    static std::string isGifSupportedErrorMessage;
    if (firstCall)
    {
        auto gifEncoder = new vir::GifEncoder();
        isGifSupported = gifEncoder->canRunOnDeviceInUse();
        if (!isGifSupported)
            isGifSupportedErrorMessage = gifEncoder->errorMessage();
        delete gifEncoder;
        firstCall = false;
    }

    static vir::GifEncoder gifEncoder;

    float x0 = ImGui::GetCursorPosX();
    ImGui::Text("Export type                 ");
    ImGui::SameLine();
    float exportButtonWidth = -1;//ImGui::GetCursorPosX()-x0+entryWidth;
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##exportTypeSelector", 
            exportTypeToName[exportType_].c_str()
        )
    )
    {
        for(auto e : exportTypeToName)
        {
            bool disabled(e.first == ExportType::GIF && !isGifSupported);
            std::string entryName = e.second;
            if (disabled)
            {
                ImGui::BeginDisabled();
                entryName = "GIF (requires OpenGL v4.3 or above)";
            }
            if (ImGui::Selectable(entryName.c_str()))
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
        ImGui::PushItemWidth(entryWidth);
        ImGui::InputFloat("##startTimeInput", &exportStartTime_);
        ImGui::PopItemWidth();
        ImGui::Text("End time                    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        ImGui::InputFloat("##endTimeInput", &exportEndTime_);
        ImGui::PopItemWidth();
        ImGui::Text("Target FPS                  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        float exportFps0(exportFps_);
        ImGui::InputFloat("##targetFpsInput", &exportFps_, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        if (exportType_ == ExportType::GIF)
        {
            if (exportFps0 != exportFps_)
                exportFps_ = 100.0f/int(100.0f/exportFps_+.5f);
            ImGui::Text("Palette bit depth           ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(Determines the GIF palette size. The number of colors in the palette is 
2^(palette bit depth))");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(entryWidth);
            ImGui::SliderInt("##paletteBitSlider", &gifPaletteBitDepth_, 2, 8);
            ImGui::PopItemWidth();
            ImGui::Text("Update palette every frame  ");
            ImGui::SameLine();
            ImGui::Checkbox
            (
                "##updatePaletteEveryFrame", 
                &updatePaletteEveryFrame_
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
            ImGui::PushItemWidth(entryWidth);
            ImGui::SliderInt
            (
                "##exportToolAlphaCutoff", 
                &gifAlphaCutoff_, 
                0, 
                255
            );
            ImGui::PopItemWidth();
            ImGui::Text("Color dithering             ");
            ImGui::SameLine();
            ImGui::PushItemWidth(entryWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##gifDitheringSelector", 
                    gifDitheringLevelToName[gifDitheringLevel_].c_str()
                )
            )
            {
                for(auto e : gifDitheringLevelToName)
                {
                    if (ImGui::Selectable(e.second.c_str()))
                    {
                        gifDitheringLevel_ = e.first;
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
    ImGui::PushItemWidth(entryWidth);
    ImGui::InputInt("##nRendersPerFrame", &nRendersPerFrame_);
    nRendersPerFrame_ = std::min(std::max(1, nRendersPerFrame_), 1000);
    ImGui::PopItemWidth();
    if (nRendersPerFrame_ > 1 && exportType_ != ExportType::Image)
    {
        ImGui::Text("Multi-pass first frame only ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##multipleRendersOnlyOnFirstFrame", 
            &multipleRendersOnlyOnFirstFrame_
        );
    }

    //--------------------------------------------------------------------------

    ImGui::Separator();
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    int nColumns = 3;
    int deleteRow = -1;
    const float& aspectRatio
    (
        vir::GlobalPtr<vir::Window>::instance()->aspectRatio()
    );
    bool exportResolutionChanged = false;
    if (ImGui::BeginTable("##exportResolutionTable", nColumns, flags))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("Layer name", flags, 10.0f*fontSize);
        ImGui::TableSetupColumn("Export resolution", flags, 10.0f*fontSize);
        float width = isGuiInMenu_ ? 5*fontSize : ImGui::GetContentRegionAvail().x;
        ImGui::TableSetupColumn("", flags, width);
        ImGui::TableHeadersRow();
        
        // Actual rendering ----------------------------------------------------
        int nRows = (int)layers_.size();
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
                glm::ivec2 exportResolution0 = exportResolution_;
                ImGui::InputInt2
                (
                    "##i2Input", 
                    glm::value_ptr(exportResolution_)
                );
                if (exportResolution0.x != exportResolution_.x)
                {
                    exportResolution_.y = 
                        std::max(exportResolution_.x/aspectRatio, 1.f);
                    exportResolutionChanged = true;
                }
                else if (exportResolution0.y != exportResolution_.y)
                {
                    exportResolution_.x = 
                        std::max(exportResolution_.y*aspectRatio, 1.f);
                    exportResolutionChanged = true;
                }
                exportResolution_.x=std::max(exportResolution_.x, 1);
                exportResolution_.y=std::max(exportResolution_.y, 1);
                ImGui::PopItemWidth();
                ImGui::TableSetColumnIndex(col++);
                ImGui::PopID();
                continue;
            }
            
            auto layer = layers_[row-1];
            ExportLayerData& exportData = exportLayerData_[layer];
            glm::ivec2& resolution = exportData.resolution;
            auto& resolutionScale = exportData.resolutionScale;
            bool& resolutionLocked = exportData.resolutionLocked;

            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            ImGui::Text(layer->name().c_str());
            ImGui::PopItemWidth();

            // Column 1 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (exportResolutionChanged && !resolutionLocked)
                resolution = (glm::vec2)exportResolution_*resolutionScale + 
                    glm::vec2({.5,.5});
            if (layer->rendersTo() != Layer::RendersTo::Window)
            {
                glm::ivec2 resolution0 = resolution;
                ImGui::InputInt2("##i2Input", glm::value_ptr(resolution));
                if (resolution0.x != resolution.x)
                {
                    resolution.y = 
                        std::max(resolution.x/layer->aspectRatio(), 1.f);
                    resolutionScale.x = //(aspectRatio > 1.0) ? 
                        //(float)resolution.x/exportResolution_.x :
                        (float)resolution.y/exportResolution_.y;
                    if (layer->isAspectRatioBoundToWindow())
                        resolutionScale.y = resolutionScale.x;
                }
                else if (resolution0.y != resolution.y)
                {
                    resolution.x = 
                        std::max(resolution.y*layer->aspectRatio(), 1.f);
                    resolutionScale.y = //(aspectRatio > 1.0) ? 
                        //(float)resolution.x/exportResolution_.x :
                        (float)resolution.y/exportResolution_.y;
                    if (layer->isAspectRatioBoundToWindow())
                        resolutionScale.x = resolutionScale.y;
                }
            }
            else
            {
                resolution = exportResolution_;
                ImGui::BeginDisabled();
                ImGui::InputInt2("##i2Input", glm::value_ptr(resolution));
                ImGui::EndDisabled();
            }
            ImGui::PopItemWidth();

            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            if (layer->rendersTo() == Layer::RendersTo::Window)
            {
                ImGui::PopID();
                continue;
            }

            ImGui::PushItemWidth(-1);
            bool resolutionLocked0(resolutionLocked);
            if (ImGui::Button(resolutionLocked?"Unlock":"Lock", ImVec2(-1,0)))
                resolutionLocked = !resolutionLocked;
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(When the lock is active, the export resolution of this layer will not be 
modified upon changes to the output export resolution. Nonetheless, the
export resolution of this layer may still be modified, although the aspect 
ratio is always locked to the aspect ratio of the source layer)"
                );
                ImGui::EndTooltip();
            }
            if (!resolutionLocked && resolutionLocked0)
                resolution = (glm::vec2)exportResolution_*resolutionScale + 
                    glm::vec2({.5,.5});
            ImGui::PopItemWidth();
            
            ImGui::PopID();

            resolution.x=std::max(resolution.x, 1);
            resolution.y=std::max(resolution.y, 1);
        }
        ImGui::EndTable();
    }

    //--------------------------------------------------------------------------

    ImGui::Separator();
    if (!isExporting_)
        exportButtonGui("Export", "##exportImage", {exportButtonWidth, 0.0});
    else
    {
        ImGui::BeginDisabled();
        exportButtonGui("Exporting...","##exportImage",{exportButtonWidth,0.0});
        ImGui::EndDisabled();
    }
    if (isExporting_)
        ImGui::ProgressBar
        (
            exportProgress(), 
            ImVec2(-1, 0.0f)
        );

    if (!isGuiInMenu_)
        ImGui::End();
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

void ExportTool::exportButtonGui
(
    const char* name, 
    const char* dialogKey, 
    const ImVec2& size
)
{
    std::string lastOpenedPath(".");
    static int sIndex;
    if (ImGui::Button(name, size))
    {
        ImGui::SetNextWindowSize(ImVec2(800,400), ImGuiCond_FirstUseEver);
        ImGuiFileDialog::Instance()->OpenDialog
        (
            dialogKey, 
            "Export to", 
            (
                exportType_ == ExportType::Image || 
                exportType_ == ExportType::VideoFrames
            ) ? 
            ".png" :
            //(exportType_ == ExportType::Video) ?
            //".avi" : 
            ".gif", 
            lastOpenedPath
        );
    }
    if (ImGuiFileDialog::Instance()->Display(dialogKey)) 
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            lastOpenedPath = 
                ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
            isExporting_ = true;
            std::string filepath = 
                ImGuiFileDialog::Instance()->GetFilePathName();

            // A very peculiar approach...
            exportFileExtension_ = "";
            exportFilepathNoExtension_ = "";
            bool foundDot(false);
            for (int i=filepath.size()-1; i>=0; i--)
            {
                char& c(filepath[i]);
                if (!foundDot)
                {
                    foundDot = (c == '.');
                    exportFileExtension_ = c+exportFileExtension_;
                }
                else
                    exportFilepathNoExtension_ = c+exportFilepathNoExtension_;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

}