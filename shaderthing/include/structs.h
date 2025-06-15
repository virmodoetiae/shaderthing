#pragma once

#include <string>
#include <vector>
#include "shaderthing/include/macros.h"
#include "shaderthing/include/oo/deferredactionbuffer.h"
#include "shaderthing/include/typedefs.h"
#include "thirdparty/imgui/imgui.h"

// Forwards

namespace vir
{
class TiledQuad;
class FrameBuffer;
class Shader;
}

namespace ShaderThing
{

// Forwards
class TextEditor;

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

struct Font
{
    ImFont*      imFont                    = nullptr;
    float*       scale                     = nullptr;
    ImFontConfig imFontConfig              = {}; 
    bool         isJapaneseLoaded          = false;
    bool         isSimplifiedChineseLoaded = false;
};

struct Renderer
{
    float        lowerFpsLimit           = 5.0;
    bool         isVSyncEnabled          = true;
    bool         isTiledRenderingEnabled = false;
    unsigned int tileIndex               = 0;
    unsigned int nTiles                  = 1;
    unsigned int nTilesCache;
};

struct RenderResult
{
    bool renderPassesComplete;
    bool flipWindowBuffer;
};

struct SharedUniforms
{
    
};

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
        Target                 target              = Target::Window;
        UPtr<vir::TiledQuad>   quad;
        UPtr<vir::Framebuffer> framebufferA;
        UPtr<vir::Framebuffer> framebufferB;
        vir::Framebuffer*      frontFramebuffer    = nullptr;
        vir::Framebuffer*      backFramebuffer     = nullptr;
        vir::Framebuffer*      resourceFramebuffer = nullptr;
        UPtr<vir::Shader>      shader;

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
               bool rename                        = false;
               bool isDeletionConfirmationPending = false;
               bool uncompiledChanges             = false;
               bool isAspectRatioBoundToWindow    = true;
               bool rescaleWithWindow             = true;
        static bool requestRecompilation;
        static bool restartRendering;
    };

    const unsigned int     id;
    const std::string      imGuiMenuId;
    const std::string      imGuiTabId;
          std::string      name;
          
          glm::vec2        resolution;
          glm::vec2        resolutionRatio = {1.f, 1.f};
          float            aspectRatio;
          float            depth;
          Renderer         renderer;
          UPtr<TextEditor> sourceEditor;
          std::string      sourceHeader;
          std::string      headerErrors;
          unsigned int     activeGuiTabId = 0;
          Flags            flags;
    
    Layer(unsigned int aId) : 
        id(aId), 
        imGuiMenuId("menuLayer"+std::to_string(id)), 
        imGuiTabId("tabLayer"+std::to_string(id)) {}
    NO_COPY(Layer)
};

struct AppData
{
    std::string          controlPanelTitle = "Control panel###CP";
    Project              project;
    Font                 font;
    Renderer             renderer;
    UPtrVector<Layer>    layers;
    UPtr<TextEditor>     sharedSourceEditor;
    DeferredActionBuffer deferredActionBuffer;
};

}
