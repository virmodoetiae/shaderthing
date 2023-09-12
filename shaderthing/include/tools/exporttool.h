#ifndef ST_EXPORT_TOOL_H
#define ST_EXPORT_TOOL_H

#include <vector>

#include "vir/include/vir.h"

namespace ShaderThing
{

class ShaderThingApp;
class Layer;

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
        float resolutionScale = 1.0;
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

    int nRendersPerFrame_;
    bool multipleRendersOnlyOnFirstFrame_;

    void exportButtonGui
    (
        const char* name, 
        const char* dialogKey, 
        const ImVec2& size
    );

public:

    ExportTool(ShaderThingApp& app);
    ~ExportTool();

    void reset();

    void loadState(std::string& source, uint32_t& index);
    void saveState(std::ofstream& file);
    void update();
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();
    void exportFrame();

    // Accessors
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
};

}

#endif