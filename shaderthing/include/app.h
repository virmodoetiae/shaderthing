/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <random>
#include <string>
#include <vector>
#include <unordered_map>

#include "vir/include/vmacros.h"
#include "vir/include/vgraphics/vcore/vuniform.h"
#include "thirdparty/imgui/imgui.h"

#include "shaderthing/include/deferredactionbuffer.h"
#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/texteditor.h"
#include "shaderthing/include/typedefs.h"
#include "shaderthing/include/uniform.h"

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

struct ExportState
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
        bool        outputResolutionChanged         = false;
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

    void initialize(RenderState& renderState, ExportState& exportState);
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

struct RenderResult
{
    bool renderPassesComplete;
    bool flipWindowBuffer;
};

//----------------------------------------------------------------------------//

class App
{
private:
    std::string          controlPanelTitle_           = "Control panel###CP";
    Font                 font_;
    Project              project_;
public:
    DeferredActionBuffer deferredActionBuffer;
    FileDialog           fileDialog;
    RenderState          renderState;
    ExportState          exportState;
    UPtr<SharedStorage>  sharedStorage;
    UPtr<SharedUniforms> sharedUniforms;
    UPtrVector<Layer>    layers;
    bool                 layersHaveUncompiledEdits   = false;
    bool                 layersHaveCompilationErrors = false;
    UPtrVector<Resource> resources;
public:

    App();

    // Logic -------------------------------------------------------------------

    void initialize();

    void createNewLayer(bool compileShader=true);

    void preRenderUpdate();

    RenderResult renderShaders
    (
        vir::Framebuffer* target, 
        const unsigned int nRenderPasses
    );
    void postRenderUpdate();
    
    void setWindowResolution
    (
        glm::ivec2 resolution, 
        const bool windowFrameManuallyDragged,
        const bool prepareForExport = false
    );

    void setRenderingTiles(int nTiles);

    void addLayerToResources(WPtr<Layer> layer);

    void removeLayerFromResources(WPtr<Layer> layer);

    void toggleRenderingPaused(bool dueToFlowFps);

    void toggleKeyboardInputs();

    void toggleMouseInputs();

    void toggleCameraMouseInputs();

    void toggleCameraKeyboardInputs();

    void setMouseInputsClamped(bool flag);

    void setMouseCaptured(bool flag);

    void saveTo
    (
        const std::string& filepath, 
        bool triggeredByAutosave = false
    );

    void loadFrom
    (
        const std::string& filepathOrData,
        bool fromMemory
    );

    void updateLayersDueToUniformTypeOrNameChanged
    (
        UPtr<Uniform>& uniform, 
        bool recompileShaders = false
    );

    void updateLayersDueToUniformDeletion
    (
        UPtr<Uniform>& uniform
    );

    // GUI ---------------------------------------------------------------------

    void renderControlPanel();

    void renderMenuBar();

    void renderLayersTabBar();

    void renderUniformsTab(Layer* layer);

    // Render the default/built-in shared uniforms only as a table and return the
    // row count
    int renderBuiltInSharedUniforms();

    // Render the button for editing uniform bounds, returns a flag indicating 
    // whether the bounds have been changed during this frame
    bool renderEditUniformBoundsButton
    (
        const vir::Ptr<Uniform>& uniform,
        bool renderDragStepSlider = false
    );

    // Render the GUI of the provided uniform at the provided table row and return
    // true if the uniform type is changed by user interation with this GUI, else
    // false
    bool renderUniformTableRow
    (
        UPtr<Uniform>& uniform,
        Layer* layer,
        int row,
        const bool showSeparator = false,
        const bool showSharedAndDefaultUniforms = true
    );

    void renderResourcesMenuItem();

    void renderResourcesTable();

    void renderResourcesTableRow(int row);

    void renderAddResourceButton(int row);

    void renderResourceActionsButton(int row);
};

//----------------------------------------------------------------------------//

}