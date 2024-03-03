#pragma once

#include <string>
#include <vector>

#include "shaderthing-p/include/filedialog.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

class ShaderThingApp;
class Layer;
class ObjectIO;

typedef vir::Quantizer::Settings::DitherMode DitherMode;

class Exporter : vir::Event::Receiver
{
public:

    enum class ExportType
    {
        Image,
        GIF,
        VideoFrames
    };

private:

    struct Settings
    {
        glm::ivec2    outputResolution;
        float         outputResolutionScale           = 1.f;
        bool          outputResolutionChanged         = false;
        unsigned int  nRenderPasses                   = 1;
        bool          areRenderPassesOnFirstFrameOnly = false;
        float         startTime                       = 0.f;
        float         endTime                         = 1.f;
        float         fps                             = 60.f;
        bool          isGifPaletteDynamic             = true;
        unsigned int  gifPaletteBitDepth              = 8;
        unsigned int  gifAlphaCutoff                  = 255;
        DitherMode    gifDitherMode                   = DitherMode::None;
    };
    Settings          settings_                       = {};

    ExportType        exportType_                     = ExportType::Image;
    vir::Framebuffer* framebuffer_                    = nullptr;
    vir::GifEncoder*  gifEncoder_                     = nullptr;
    FileDialog        fileDialog_;

    bool              isRunning_                      = false;
    unsigned int      frame_                          = 0;
    unsigned int      nFrames_                        = 0;
    unsigned int      renderPass_                     = 0;

    void exportButtonGUI();

public:

    Exporter();
    ~Exporter();

    DECLARE_RECEIVABLE_EVENTS
    (
        vir::Event::Type::WindowResize
    );
    virtual void onReceive(vir::Event::WindowResizeEvent& event) override;

    void update();

    void renderGUI(const std::vector<Layer*>& layers);

    bool isRunning() const {return isRunning_;}
    vir::Framebuffer* framebuffer() const {return framebuffer_;}
};

}