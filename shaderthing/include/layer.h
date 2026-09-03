/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <random>
#include <string>
#include <vector>
#include <unordered_map>
#include "shaderthing/include/deferredactionbuffer.h"
#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/texteditor.h"
#include "shaderthing/include/uniform.h"
#include "shaderthing/include/typedefs.h"
#include "vir/include/vmacros.h"
#include "vir/include/vgraphics/vcore/vuniform.h"
#include "thirdparty/imgui/imgui.h"

namespace ShaderThing
{

struct AppState;
struct SharedUniforms;

class Layer : public UniformContainer, public vir::EnableWeakFromThis<Layer>
{
friend LayerResource;
public :

    // Data --------------------------------------------------------------------

    struct RenderState
    {
        enum class Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
        Target                   target              = Target::Window;
        enum class TilingDirection
        {
            Horizontal,
            Vertical
        };
        TilingDirection          tilingDirection;
        unsigned int             nTiles              = 1;
        UPtr<vir::TiledQuad>     quad;
        UPtr<vir::Framebuffer>   framebufferA;
        UPtr<vir::Framebuffer>   framebufferB;
        vir::Framebuffer*        frontFramebuffer    = nullptr;
        vir::Framebuffer*        backFramebuffer     = nullptr;
        vir::Framebuffer*        resourceFramebuffer = nullptr;
        UPtr<vir::Shader>        shader;
        WPtr<Uniform>            iAspectRatioUniform;
        WPtr<Uniform>            iResolutionUniform;
        static UPtr<vir::Shader> textureMapperShader;
    };

    struct ExportSettings
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
        std::map<Uniform*, std::string> uninitializedResourceLayers;
    };

    static const unsigned int nMaxLayers = 32;

public:
    const unsigned int   id;
    const std::string    imGuiMenuId;
    const std::string    imGuiTabId;
protected:
          AppState&      appState_;
          unsigned int   activeGuiTabId_                = 0;
          std::string    name_;
          bool           isAspectRatioBoundToWindow_    = true;
          bool           rescaleWithWindow_             = true;
          glm::vec2      resolution_;
          glm::vec2      resolutionRatio_               = {1.f, 1.f};
          float          aspectRatio_;
          float          depth_;
          RenderState    renderState_;
          TextEditor     sourceEditor_;
static const std::string defaultSharedSource_;
   static TextEditor     sharedSourceEditor_;
          std::string    sourceHeader_;
          std::string    headerErrors_;
public:
          bool           hasUncompiledEdits            = false;
          bool           isDeletionConfirmationPending = false;
          ExportSettings exportSettings;
          Cache          cache;
    
protected:

    // Ctor/creator/dtor -------------------------------------------------------

    Layer(unsigned int id, AppState& appState);

public:

    static UPtr<Layer> create(unsigned int id, AppState& appState);

    static UPtr<Layer> loadFrom
    (
        ObjectIO& io, 
        unsigned int id, 
        AppState& appState
    );

    virtual ~Layer();

    // Getters -----------------------------------------------------------------

    const std::string& name() const {return name_;}

    const std::string* namePtr() const {return &name_;}

    const glm::vec2& resolution() const {return resolution_;}

    const float& depth() const {return depth_;}

    RenderState::Target renderingTarget() const {return renderState_.target;}

    std::string fragmentShaderSourceHeader() const;

    std::string vertexShaderSource() const;

    static std::string vertexShaderSource(const SharedUniforms& sharedUniforms);

    bool hasCompilationErrors() const;

    // Setters -----------------------------------------------------------------

    void setResolution
    (
        glm::ivec2 resolution,
        const bool windowFrameManuallyDragged,
        const bool tryEnfoceWindowAspectRatio = false,
        const bool setExportResolution = true
    );

    void setDepth(const float depth);

    void setFramebufferWrapMode(int i, WrapMode mode);

    void setFramebufferMagFilterMode(FilterMode mode);

    void setFramebufferMinFilterMode(FilterMode mode);

    // Logic -------------------------------------------------------------------

    void rebuildFramebuffers
    (
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    );

    void clearFramebuffers();

    bool compileShader(bool setBlankShaderOnError = false);

    void setRenderingTilesNumber(unsigned int nTiles);

    void renderShader(vir::Framebuffer* target, const bool clearTarget);

    void saveTo(ObjectIO& io);

    static void resetSharedSourceEditor();

    static void resetSharedSourceEditor(const std::string& content);
    
    // GUI ---------------------------------------------------------------------

    void renderMenuItemGui();

    void renderFramebufferSettingsGui();

    void renderTabGui();

    static void renderSharedCompilationErrorsGui();

    void renderCompilationErrorsGui();

};

}