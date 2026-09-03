#pragma once

#include <random>
#include <string>
#include <vector>
#include <unordered_map>
#include "shaderthing/include/deferredactionbuffer.h"
#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/texteditor.h"
#include "shaderthing/include/uniform.h"
#include "shaderthing/include/typedefs.h"
#include "vir/include/vmacros.h"
#include "vir/include/vgraphics/vcore/vuniform.h"
#include "thirdparty/imgui/imgui.h"

// Forwards

namespace vir
{
class TiledQuad;
class Shader;
class Camera;
}

namespace ShaderThing
{

//----------------------------------------------------------------------------//

struct Exporter
{
    enum class ExportType
    {
        Image,
        GIF,
        VideoFrames
    };
    ExportType             exportType      = ExportType::Image;
    
    UPtr<vir::Framebuffer> framebuffer;
    unsigned char*         framebufferData = nullptr;
    vir::GifEncoder*       gifEncoder      = nullptr;
    FileDialog             fileDialog;

    bool         isActive                        = false;
    bool         isAveragedPaletteReady          = false;
    bool         areRenderPassesOnFirstFrameOnly = false;
    bool         resetFrameCounterAfterExport    = true;
    unsigned int frameIndex                      = 0;
    unsigned int nFrames                         = 0;
    unsigned int nRenderPasses                   = 1;
    float        startTime                       = 0.f;
    float        endTime                         = 1.f;
    float        timeStep                        = 0.f;
    float        fps                             = 60.f;
    float        outputResolutionScale           = 1.0;
    std::string  outputFilepath;
    glm::vec2    outputResolution                = {0, 0};
    PaletteMode  gifPaletteMode                  = PaletteMode::Dynamic;
    unsigned int gifPaletteBitDepth              = 8;
    unsigned int gifAlphaCutoff                  = 0;
    DitherMode   gifDitherMode                   = DitherMode::None;
    
    struct Cache
    {
        std::string outputFilepathExtended;
    };
    struct Toggles
    {
        bool         outputResolutionChanged         = false;
    };
    Cache        cache                           = {};
    Toggles      toggles                         = {};
};

//----------------------------------------------------------------------------//

struct Font
{
    ImFont*      imFont                    = nullptr;
    float*       scale                     = nullptr;
    ImFontConfig imFontConfig              = {}; 
    bool         isJapaneseLoaded          = false;
    bool         isSimplifiedChineseLoaded = false;
};

//----------------------------------------------------------------------------//

struct SharedUniforms
{
public :
    // Fixed camera used to retrieve the value of the projection view 
    // matrix iMVP
    UPtr<vir::Camera>        screenCamera;
    // Movable camera which responds to keyboard and mouse controls and is
    // used to provide values to iWASD, iLook uniforms
    UPtr<vir::Camera>        shaderCamera;
    // Random number generator
    std::mt19937_64          rndGenerator;
    
    float      iTime          = 0.f;
    float      iTimeDelta     = 0.f;
    float      iRandom        = 0.f;
    bool       iUserAction    = false;
    glm::vec3  iWASD          = {0,0,-1};
    glm::vec3  iLook          = {0,0,1};
    glm::vec4  iMouse         = {0,0,0,0};
    float      iAspectRatio   = 1.f;
    glm::vec2  iResolution    = {512,512};
    glm::ivec3 iKeyboard[256] = {};

    UniformContainer fragment;
    WPtr<Uniform> iFrameUniform;
    WPtr<Uniform> iRenderPassUniform;
    WPtr<Uniform> iTimeUniform;
    WPtr<Uniform> iTimeDeltaUniform;
    WPtr<Uniform> iRandomUniform;
    WPtr<Uniform> iUserActionUniform;
    WPtr<Uniform> iExportUniform;
    WPtr<Uniform> iWASDUniform;
    WPtr<Uniform> iLookUniform;
    WPtr<Uniform> iMouseUniform;
    WPtr<Uniform> iAspectRatioUniform;
    WPtr<Uniform> iResolutionUniform;
    WPtr<Uniform> iKeyboardUniform;

    UniformContainer vertex;
    WPtr<Uniform> iMVPUniform;

    const unsigned int userUniformsStartIndex = 13;

    bool isTimePaused                       = false;
    bool isTimePausedBecauseRenderingPaused = false;
    bool isTimeLooped                       = false;
    bool isTimeResetOnFrameCounterReset     = true;
    bool isTimeDeltaSmooth                  = false;
    bool isRandomNumberGeneratorPaused      = false;
    bool isKeyboardInputEnabled             = true; // iKeyboard
    bool isMouseInputEnabled                = true; // iMouse
    bool isMouseInputClampedToWindow        = false;
    bool mouseInputRequiresLMBHold          = true;
    bool isCameraKeyboardInputEnabled       = true; // iWASD
    bool isCameraMouseInputEnabled          = true; // iLook
    bool cameraMouseInputRequiresLMBHold    = true;

    struct Toggles
    {
        bool updateDataRangeII                  = false;
        bool stepToNextTimeStep                 = false;
    };
    Toggles toggles = {};
};

//----------------------------------------------------------------------------//

/*

struct Layer : public UniformContainer
{
public :
    struct Rendering
    {
        enum class Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
        Target                          target              = Target::Window;
        UPtr<vir::TiledQuad>            quad;
        UPtr<vir::Framebuffer>          framebufferA;
        UPtr<vir::Framebuffer>          framebufferB;
        vir::Framebuffer*               frontFramebuffer    = nullptr;
        vir::Framebuffer*               backFramebuffer     = nullptr;
        vir::Framebuffer*               resourceFramebuffer = nullptr;
        UPtr<vir::Shader>               shader;
        WPtr<Uniform>                   iAspectRatioUniform;
        WPtr<Uniform>                   iResolutionUniform;
        static UPtr<vir::Shader>        textureMapperShader;

        struct Tiles
        {
            enum class Direction
            {
                Horizontal,
                Vertical
            };
            Direction    direction = Direction::Horizontal;
            unsigned int size      = 1;
        };
        Tiles tiles;
    };
    struct ExportData
    {
        enum class FramebufferClearPolicy
        {
            // The framebuffers are never cleared
            None, 
            // The framebuffers are cleared only once, when the export starts
            ClearOnFirstFrameExport,
            // The framebuffers are cleared at the beginning of every frame, but
            // not on sub-frame render passes (i.e., the framebuffers are 
            // cleared at the beginning of the first sub-frame render pass of
            // each frame)
            ClearOnEveryFrameExport
        };
        FramebufferClearPolicy clearPolicy = FramebufferClearPolicy::None;
        glm::ivec2             originalResolution;
        glm::ivec2             resolution;
        float                  resolutionScale       = 1.f;
        float                  windowResolutionScale = 1.f;
        bool                   rescaleWithOutput     = true;
    };
    struct Cache
    {
        WPtrVector<Uniform> uncompiledUniforms;
    };

    static const unsigned int nMaxLayers = 32;
    const unsigned int        id;
    const std::string         imGuiMenuId;
    const std::string         imGuiTabId;
          std::string         name;
          bool                isAspectRatioBoundToWindow    = true;
          bool                isDeletionConfirmationPending = false;
          bool                rescaleWithWindow             = true;
          bool                hasUncompiledEdits            = false;
          glm::vec2           resolution;
          glm::vec2           resolutionRatio = {1.f, 1.f};
          float               aspectRatio;
          float               depth;
          Rendering           rendering;
          TextEditor          sourceEditor;
          std::string         sourceHeader;
          std::string         headerErrors;
          unsigned int        activeGuiTabId = 0;
          ExportData          exportData;
          Cache               cache;
    
    Layer(unsigned int aId) : 
        id(aId), 
        imGuiMenuId("menuLayer"+std::to_string(id)),
        imGuiTabId("tabLayer"+std::to_string(id)) {}
    DELETE_COPY(Layer)
};

*/

//----------------------------------------------------------------------------//

struct Project
{
    std::string        filepath          = "";
    std::string        filename          = "untitled.stf";
    const std::string* exampleToBeLoaded = nullptr;
    bool               forceSaveAs       = true;
    bool               isAutoSaveEnabled = true;
    float              timeSinceLastSave = 0.f;
    float              autoSaveInterval  = 60.f;
};

//----------------------------------------------------------------------------//

struct RenderState
{
    bool              isPaused                = false;
    bool              isTiledRenderingEnabled = false;
    bool              isVSyncEnabled          = true;
    unsigned int      frameIndex              = 0;
    unsigned int      passIndex               = 0;
    unsigned int      tileIndex               = 0;
    unsigned int      nTiles                  = 1;
    unsigned int      nTilesCache;
    float             lowerFpsLimit           = 5.0;
    struct Toggles
    {
        bool stepToNextFrame                  = false;
        bool resetFrameCounterPreOrPostExport = true;
        bool requestFullRecompilation         = false;
        bool tiledRenderingPauseRequested     = false;
    };
    Toggles           toggles                 = {};
};

//----------------------------------------------------------------------------//

struct RenderResult
{
    bool renderPassesComplete;
    bool flipWindowBuffer;
};

//----------------------------------------------------------------------------//

struct AppState
{
    std::string          controlPanelTitle           = "Control panel###CP";
    DeferredActionBuffer deferredActionBuffer;
    FileDialog           fileDialog;
    Font                 font;
    Project              project;
    RenderState          renderState;
    UPtr<Exporter>       exporter;
    UPtr<SharedStorage>  sharedStorage;
    UPtr<SharedUniforms> sharedUniforms;
    UPtrVector<Layer>    layers;
    bool                 layersHaveUncompiledEdits   = false;
    bool                 layersHaveCompilationErrors = false;
    UPtrVector<Resource> resources;
    
};

//----------------------------------------------------------------------------//

}