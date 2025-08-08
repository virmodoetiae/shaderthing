#pragma once

#include <string>
#include <vector>
#include "shaderthing/include/macros.h"
#include "shaderthing/include/oo/deferredactionbuffer.h"
#include "shaderthing/include/oo/filedialog.h"
#include "shaderthing/include/oo/sharedstorage.h"
#include "shaderthing/include/oo/texteditor.h"
#include "shaderthing/include/typedefs.h"
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
    struct Settings
    {
        std::string  outputFilepath;
        glm::vec2    outputResolution                = {0, 0};
        float        outputResolutionScale           = 1.0;
        bool         outputResolutionChanged         = false;
        unsigned int nRenderPasses                   = 1;
        bool         areRenderPassesOnFirstFrameOnly = false;
        bool         resetFrameCounterAfterExport    = true;
        float        startTime                       = 0.f;
        float        endTime                         = 1.f;
        float        fps                             = 60.f;
        PaletteMode  gifPaletteMode                  = PaletteMode::Dynamic;
        unsigned int gifPaletteBitDepth              = 8;
        unsigned int gifAlphaCutoff                  = 0;
        DitherMode   gifDitherMode                   = DitherMode::None;
    };
    Settings settings = {};

    struct Cache
    {
        std::string outputFilepathExtended;
    };
    Cache cache = {};

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

    bool         isActive               = false;
    bool         isAveragedPaletteReady = false;
    unsigned int frame                  = 0;
    unsigned int nFrames                = 0;
    double       timeStep               = 0.f;
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

struct Uniform : vir::Uniform
{
    typedef vir::Uniform::Type Type;

    bool        isSharedByUser         = false;
    bool        hasSharedByUserChanged = false;
    bool        isLogarithmic          = false; // For floats only

    struct GUI
    {
        bool markedForDeletion = false;

        // True if this uniform is of vec3 or vec4 and its value is set via an
        // ImGui color picker tool
        bool usesColorPicker = false;

        // True if this uniform's bounds are to be displayed in the GUI
        bool showBounds = true;

        // Numerical bounds for the value of this uniform (or its components, if
        // a multi-component vector), only used by uniform types other than
        // Type::Bool, Type::Sampler2D, Type::Cubemap, and not necessarily used
        // by uniforms which have a specialType
        glm::vec2 bounds = {0.f, 1.f};

        // Only for vec2, ivec2 type uniforms that can be set by dragging an
        // arrow over the screen. This is a scaling factor from on-screen-arrow
        // size to actual uniform value increment
        float dragStep = 1.;
        
        // For floats only: smallest (absolute) value that can be represented
        // when isLogarithmic == true and the value bounds include 0.f
        float logarithmicZero = 1e-3f;
    };
    GUI gui;
};

//----------------------------------------------------------------------------//

struct SharedUniforms
{
    struct Flags
    {
        bool updateDataRangeII                  = false;
        bool stepToNextFrame                    = false;
        bool stepToNextTimeStep                 = false;
        bool resetFrameCounter                  = true;
        bool resetFrameCounterPreOrPostExport   = true;
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
        bool tiledRenderingPauseRequested       = false;
    };
    Flags flags;

    // Fixed camera used to retrieve the value of the projection view 
    // matrix iMVP
    UPtr<vir::Camera>        screenCamera;
    // Movable camera which responds to keyboard and mouse controls and is
    // used to provide values to iWASD, iLook uniforms
    UPtr<vir::Camera>        shaderCamera;
    
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

    UPtr<Uniform> iFrameUniform;
    UPtr<Uniform> iRenderPassUniform;
    UPtr<Uniform> iTimeUniform;
    UPtr<Uniform> iTimeDeltaUniform;
    UPtr<Uniform> iRandomUniform;
    UPtr<Uniform> iUserActionUniform;
    UPtr<Uniform> iExportUniform;
    UPtr<Uniform> iWASDUniform;
    UPtr<Uniform> iLookUniform;
    UPtr<Uniform> iMouseUniform;
    UPtr<Uniform> iAspectRatioUniform;
    UPtr<Uniform> iResolutionUniform;
    UPtr<Uniform> iKeyboardUniform;
    UPtr<Uniform> iMVPUniform;
    
    UPtrVector<Uniform> userUniforms_;

    // Fragment shader shared uniform buffer
    UPtr<vir::DynamicUniformBuffer> fBuffer;
    static const unsigned int       fBufferBindingPoint = 0;
    // Vertex shader shared uniform buffer
    UPtr<vir::DynamicUniformBuffer> vBuffer;
    static const unsigned int       vBufferBindingPoint = 1;
};

//----------------------------------------------------------------------------//

struct Layer
{
    struct Renderer
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
        UPtr<vir::DynamicUniformBuffer> uniformBuffer;
        unsigned int                    uniformBufferBindingPoint;
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
    struct Flags
    {
               bool isDeletionConfirmationPending = false;
               bool uncompiledChanges             = false;
               bool isAspectRatioBoundToWindow    = true;
               bool rescaleWithWindow             = true;
        static bool requestRecompilation;
        static bool restartRendering;
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
        UPtrVector<Uniform> uncompiledUniforms;
    };

    static const unsigned int nMaxLayers = 32;
    const unsigned int        id;
    const std::string         imGuiMenuId;
    const std::string         imGuiTabId;
          std::string         name;
          
          glm::vec2           resolution;
          glm::vec2           resolutionRatio = {1.f, 1.f};
          float               aspectRatio;
          float               depth;
          Renderer            renderer;
          UPtrVector<Uniform> uniforms;
          TextEditor          sourceEditor;
          std::string         sourceHeader;
          std::string         headerErrors;
          unsigned int        activeGuiTabId = 0;
          Flags               flags;
          ExportData          exportData;
          Cache               cache;
    
    Layer(unsigned int aId) : 
        id(aId), 
        imGuiMenuId("menuLayer"+std::to_string(id)), 
        imGuiTabId("tabLayer"+std::to_string(id)) {}
    NO_COPY(Layer)
};

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

struct Renderer
{
    float             lowerFpsLimit           = 5.0;
    bool              isPaused                = false;
    bool              isTiledRenderingEnabled = false;
    bool              isVSyncEnabled          = true;
    unsigned int      frame                   = 0;
    unsigned int      renderPass              = 0;
    unsigned int      tileIndex               = 0;
    unsigned int      nTiles                  = 1;
    unsigned int      nTilesCache;
};

//----------------------------------------------------------------------------//

struct RenderResult
{
    bool renderPassesComplete;
    bool flipWindowBuffer;
};

//----------------------------------------------------------------------------//

struct AppData
{
    std::string          controlPanelTitle = "Control panel###CP";
    DeferredActionBuffer deferredActionBuffer;
    Exporter             exporter;
    Font                 font;
    Project              project;
    Renderer             renderer;
    SharedStorage        sharedStorage;
    SharedUniforms       sharedUniforms;
    TextEditor           sharedSourceEditor;
    UPtrVector<Layer>    layers;
};

//----------------------------------------------------------------------------//

}