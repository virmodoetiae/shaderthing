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

#ifndef ST_EXPORT_TOOL_H
#define ST_EXPORT_TOOL_H

#include <vector>

#include "vir/include/vir.h"

namespace ShaderThing
{

class ShaderThingApp;
class Layer;
class ObjectIO;

class ExportTool
{
public:

    enum class ExportType
    {
        Image,
        GIF,
        VideoFrames
        //Video
    };

    static std::unordered_map<ExportType, std::string> exportTypeToName;
    static std::unordered_map<int, std::string> gifDitheringLevelToName;

    struct ExportLayerData
    {
        glm::ivec2 resolution;
        glm::ivec2 backupResolution;
        glm::vec2 resolutionScale = {1.f,1.f};
        bool resolutionLocked = false;

    };

private:

    ShaderThingApp& app_;
    bool isGuiOpen_;
    bool isGuiInMenu_;
    bool isExporting_;
    ExportType exportType_;
    bool updateLayers_;
    float& time_;
    bool& timePaused_;
    float exportStartTime_;
    float exportEndTime_;
    float exportFps_;
    uint32_t frame_;
    uint32_t nPreExportFrames_;
    uint32_t nExportFrames_;
    std::vector<Layer*>& layers_;
    // Framebuffer used for rendering when the desired output resolution
    // is different than the windown resolution
    vir::Framebuffer* exportFramebuffer_;

    std::string exportFilepathNoExtension_;
    std::string exportFileExtension_;
    glm::ivec2 exportResolution_;
    std::unordered_map<Layer*, ExportLayerData> exportLayerData_;
    unsigned char* exportFramebufferData_;

    // Bit depth of the palette to be used when exporting animated images in
    // GIF format. The resulting palette size will be 2^gifPaletteBitDepth_
    int gifPaletteBitDepth_;

    // Level of dithering to be applied to the exported animated image in
    // GIF format, 0 is no dithering.
    int gifDitheringLevel_;

    //
    int gifAlphaCutoff_;

    int nRendersPerFrame_;
    bool multipleRendersOnlyOnFirstFrame_;
    bool updatePaletteEveryFrame_;

    void exportButtonGui
    (
        const char* name, 
        const char* dialogKey, 
        const ImVec2& size
    );
    void exportFrame();

public:

    ExportTool(ShaderThingApp& app);
    ~ExportTool();

    //
    void reset();
    void loadState(std::string& source, uint32_t& index);
    void saveState(std::ofstream& file);
    void saveState(ObjectIO& writer);

    void update();
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    // Accessors
    int frame() {return frame_;}
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
    bool isExporting() const {return isExporting_;}
    float exportTimeStep() const 
    {
        return exportType_ != ExportType::Image ? 1.0f/exportFps_ : 0.0f;
    }
    float exportProgress(){return (float)frame_/std::max(nExportFrames_,1u);}
    vir::Framebuffer* exportFramebuffer() {return exportFramebuffer_;}
    int nRendersPerFrame();
    
    // Setters
    void setExportResolution(glm::ivec2);
    void updateLayerResolutions();

    // Serialization
    /*
    template<typename RapidJSONWriterType>
    void saveState(RapidJSONWriterType& writer)
    {
        writer.String("exporter");
        writer.StartObject();

        writer.String("exportType");
        writer.Int((int)exportType_);

        if (exportType_ != ExportType::Image)
        {
            writer.String("startTime");
            writer.Double(exportStartTime_);

            writer.String("endTime");
            writer.Double(exportEndTime_);

            writer.String("framesPerSecond");
            writer.Double(exportFps_);

            if (exportType_ == ExportType::GIF)
            {
                writer.String("gif");
                writer.StartObject();

                writer.String("paletteBitDepth");
                writer.Int(gifPaletteBitDepth_);

                writer.String("dynamicPalette");
                writer.Bool(updatePaletteEveryFrame_);
                
                writer.String("ditheringLevel");
                writer.Int(gifDitheringLevel_);
                
                writer.String("transparencyCutoffThreshold");
                writer.Int(gifAlphaCutoff_);
                
                writer.EndObject(); // gif
            }

            writer.String("multipleRenderPassesOnlyOnFirstFrame");
            writer.Bool(multipleRendersOnlyOnFirstFrame_);
        }

        writer.String("nRenderPassesPerFrame");
        writer.Int(nRendersPerFrame_);

        if (exportFilepathNoExtension_.size() > 0)
        {
            writer.String("fileNameNoExtension");
            writer.String(exportFilepathNoExtension_.c_str());
        }

        writer.String("layerData");
        writer.StartObject();
        for (auto data : exportLayerData_)
        {
            writer.String(data.first->name().c_str());
            writer.StartObject();

            writer.String("resolution");
            writer.StartArray();
            for (int i=0; i<2; i++)
                writer.Int(data.second.resolution[i]);
            writer.EndArray();

            writer.String("backupResolution");
            writer.StartArray();
            for (int i=0; i<2; i++)
                writer.Int(data.second.backupResolution[i]);
            writer.EndArray();

            writer.String("resolutionScale");
            writer.StartArray();
            for (int i=0; i<2; i++)
                writer.Double(data.second.resolutionScale[i]);
            writer.EndArray();

            writer.String("resolutionLocked");
            writer.Bool(data.second.resolutionLocked);

            writer.EndObject(); // data.first->name().c_str()
        }
        writer.EndObject(); // layerData

        writer.EndObject(); // exporter
    }
    */
};

}

#endif