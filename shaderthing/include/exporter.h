#pragma once

#include <string>
#include <vector>

#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/macros.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

class SharedUniforms;
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

    struct UpdateArgs
    {
        float& timeStep;
    };

private:

    struct Settings
    {
        std::string   outputFilepath;
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

    struct Cache
    {
        std::string   outputFilepathExtended;
    };
    Cache             cache_                          = {};

    ExportType        exportType_                     = ExportType::Image;
    vir::Framebuffer* framebuffer_                    = nullptr;
    unsigned char*    framebufferData_                = nullptr;
    vir::GifEncoder*  gifEncoder_                     = nullptr;
    FileDialog        fileDialog_;

    bool              isRunning_                      = false;
    unsigned int      frame_                          = 0;
    unsigned int      nFrames_                        = 0;
    double            timeStep_                       = 0.f;

    void exportButtonGui();

    DELETE_COPY_MOVE(Exporter)

public:

    Exporter();
    ~Exporter();

    DECLARE_RECEIVABLE_EVENTS
    (
        vir::Event::Type::WindowResize
    );
    virtual void onReceive(vir::Event::WindowResizeEvent& event) override;

    void update
    (
        SharedUniforms& sharedUniforms,
        const std::vector<Layer*>& layers,
        const UpdateArgs& args
    );

    void renderGui
    (
        SharedUniforms& sharedUniforms, 
        const std::vector<Layer*>& layers
    );

    bool isRunning() const {return isRunning_;}
    unsigned int nRenderPasses() const {return settings_.nRenderPasses;}
    vir::Framebuffer* framebuffer() const {return framebuffer_;}
};

}