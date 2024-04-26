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

#pragma once

#include <string>
#include <vector>

#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/macros.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

class Layer;
class ObjectIO;
class Resource;
class SharedUniforms;

typedef vir::Quantizer::Settings::DitherMode DitherMode;
typedef vir::GifEncoder::PaletteMode PaletteMode;

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
        std::string    outputFilepath;
        glm::vec2      outputResolution                = {0, 0};
        bool           outputResolutionChanged         = false;
        unsigned int   nRenderPasses                   = 1;
        bool           areRenderPassesOnFirstFrameOnly = false;
        bool           resetFrameCounterAfterExport    = true;
        float          startTime                       = 0.f;
        float          endTime                         = 1.f;
        float          fps                             = 60.f;
        PaletteMode    gifPaletteMode                  = PaletteMode::Dynamic;
        unsigned int   gifPaletteBitDepth              = 8;
        unsigned int   gifAlphaCutoff                  = 0;
        DitherMode     gifDitherMode                   = DitherMode::None;
    };
    Settings           settings_                       = {};

    struct Cache
    {
        std::string    outputFilepathExtended;
    };
    Cache              cache_                          = {};

    ExportType         exportType_                     = ExportType::Image;
    vir::Framebuffer*  framebuffer_                    = nullptr;
    unsigned char*     framebufferData_                = nullptr;
    vir::GifEncoder*   gifEncoder_                     = nullptr;
    FileDialog         fileDialog_;

    bool               isRunning_                      = false;
    bool               isAveragedPaletteReady_         = false;
    unsigned int       frame_                          = 0;
    unsigned int       nFrames_                        = 0;
    double             timeStep_                       = 0.f;

    void exportButtonGui(bool disabled = false);

    DELETE_COPY_MOVE(Exporter)

public:

    Exporter();
    ~Exporter();

    void save(ObjectIO& io);
    static void load(const ObjectIO& io, Exporter*& exporter);

    DECLARE_RECEIVABLE_EVENTS
    (
        vir::Event::Type::WindowResize
    );
    virtual void onReceive(vir::Event::WindowResizeEvent& event) override;

    void update
    (
        SharedUniforms& sharedUniforms,
        const std::vector<Layer*>& layers,
        const std::vector<Resource*>& resources
    );

    void writeOutput();

    void renderGui
    (
        SharedUniforms& sharedUniforms, 
        const std::vector<Layer*>& layers
    );

    bool isRunning() const {return isRunning_;}
    float timeStep() const {return timeStep_;}
    unsigned int nRenderPasses() const {return settings_.nRenderPasses;}
    vir::Framebuffer* framebuffer() const {return framebuffer_;}
};

}