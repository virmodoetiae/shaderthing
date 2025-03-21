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

#include <memory>
#include <string>
#include <vector>

#include "shaderthing/include/macros.h"
#include "shaderthing/include/texteditor.h"

#include "thirdparty/glm/glm.hpp"

#include "vir/include/vir.h"

namespace ShaderThing
{

typedef vir::TextureBuffer::WrapMode   WrapMode;
typedef vir::TextureBuffer::FilterMode FilterMode;


class ObjectIO;
class PostProcess;
class Resource;
class LayerResource;
class SharedStorage;
class SharedUniforms;
class Uniform;

class Layer : vir::Event::Receiver
{
friend LayerResource;
friend PostProcess;
friend Uniform;
friend Resource;
public:
    struct Rendering
    {
        enum class Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
        Target                          target              = Target::Window;
        vir::TiledQuad*                 quad                = nullptr;
        vir::Framebuffer*               framebufferA        = nullptr;
        vir::Framebuffer*               framebufferB        = nullptr;
        // Only framebuffers A and B are allocated. All other framebuffer ptrs
        // are either equal to framebufferA or framebufferB
        vir::Framebuffer*               frontFramebuffer    = nullptr;
        vir::Framebuffer*               backFramebuffer     = nullptr;
        vir::Framebuffer*               resourceFramebuffer = nullptr;
        vir::Shader*                    shader              = nullptr;
        std::vector<PostProcess*>       postProcesses       = {};

        struct TileData
        {
            enum class Direction
            {
                Horizontal,
                Vertical
            };
            Direction                   direction   = Direction::Horizontal;
            unsigned int                size        = 1;
        };
        TileData                        tiles;

        struct TileController
        {
            static bool                 tiledRenderingEnabled;
            static unsigned int         tileIndex;
            static unsigned int         nTiles;
            static unsigned int         nTilesCache;
        };

        struct Result
        {
            bool renderPassesComplete;
            bool flipWindowBuffer;
        };
        
        // I only use unique_ptrs to conveniently manage the lifetime of static
        // ptr-type resources
        static std::unique_ptr<vir::Shader>
                                        textureMapperShader;
        static std::unique_ptr<SharedStorage> 
                                        sharedStorage;
    };
    struct GUI
    {
               std::string              name;
               std::string              newName;
               std::string              sourceHeader;
               std::string              headerErrors;
               TextEditor               sourceEditor;
        static std::string              defaultSharedSource;
        static TextEditor               sharedSourceEditor;
               unsigned int             activeTabId                = 0;
    };
    struct Flags
    {
               bool                     rename                     = false;
               bool                     pendingDeletion            = false;
               bool                     uncompiledChanges          = false;
               bool                     isAspectRatioBoundToWindow = true;
               bool                     rescaleWithWindow          = true;
        static bool                     requestRecompilation;
        static bool                     restartRendering;
    };
    struct Cache
    {
        std::vector<Uniform*>           uncompiledUniforms;
        std::map<Uniform*, std::string> uninitializedResourceLayers;
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
        FramebufferClearPolicy          clearPolicy           = FramebufferClearPolicy::None;
        glm::ivec2                      originalResolution;
        glm::ivec2                      resolution;
        float                           resolutionScale       = 1.f;
        float                           windowResolutionScale = 1.f;
        bool                            rescaleWithOutput     = true;
    };

private:

    const uint32_t                      id_;
          glm::ivec2                    resolution_;
          glm::vec2                     resolutionRatio_ = {1.f, 1.f};
          float                         aspectRatio_;
          float                         depth_;
          std::vector<Uniform*>         uniforms_;
          Rendering                     rendering_;
          GUI                           gui_;
          Flags                         flags_;
          Cache                         cache_;
          ExportData                    exportData_;

    //------------------------------------------------------------------------//

    static unsigned int findFreeId(const std::vector<Layer*>& layers);
    static std::string glslDirectives();
    static std::string vertexShaderSource
    (
        const SharedUniforms& sharedUniforms
    );
    std::tuple<std::string, unsigned int> 
        fragmentShaderHeaderSourceAndLineCount
        (
            const SharedUniforms& sharedUniforms
        ) const;
    void setResolution
    (
        const glm::ivec2& resolution,
        const bool windowFrameManuallyDragged,
        const bool tryEnfoceWindowAspectRatio=false,
        const bool setExportResolution=true
    );
    void setName(const std::string& name);
    void setDepth(const float depth);
    void setFramebufferWrapMode(int index, WrapMode mode);
    void setFramebufferMagFilterMode(FilterMode mode);
    void setFramebufferMinFilterMode(FilterMode mode);
    void rebuildFramebuffers
    (
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    );
    void clearFramebuffers();
    void save(ObjectIO& io) const;
    static Layer* load
    (
        const ObjectIO& io,
        const std::vector<Layer*>& layers,
        const SharedUniforms& sharedUniforms,
        std::vector<Resource*>& resources
    );

    DELETE_COPY_MOVE(Layer)

public:

    static constexpr unsigned int nMaxLayers = 32;

    Layer
    (
        const std::vector<Layer*>& layers,
        const SharedUniforms& sharedUniforms,
        const bool compileShader = true
    );
    ~Layer();
    
    static void saveAll(const std::vector<Layer*>& layers, ObjectIO& io);
    static void loadAll
    (
        const ObjectIO& io,
        std::vector<Layer*>& layers,
        SharedUniforms& sharedUniforms,
        std::vector<Resource*>& resources
    );

    DECLARE_RECEIVABLE_EVENTS(vir::Event::Type::WindowResize)
    void onReceive(vir::Event::WindowResizeEvent& event) override;

    static void prepareForExport(const std::vector<Layer*>& layers);
    static void resetAfterExport(const std::vector<Layer*>& layers);

    bool removeResourceFromUniforms(const Resource* resource);
    
    bool compileShader
    (
        const SharedUniforms& sharedUniforms, 
        bool setBlankShaderOnError=false
    );
    void renderShader
    (
        vir::Framebuffer* target, 
        const bool clearTarget, 
        const SharedUniforms& sharedUniforms
    );
    void renderInternalFramebufferToTarget
    (
        vir::Framebuffer* target, 
        const bool clearTarget
    );
    static Rendering::Result renderShaders
    (
        const std::vector<Layer*>& layers,
        vir::Framebuffer* target, 
        SharedUniforms& sharedUniforms,
        const unsigned int nRenderPasses = 1
    );

    void renderFramebufferPropertiesGui();
    void renderPropertiesMenuGui(std::vector<Resource*>& resources);
    void renderTabBarGui
    (
        const std::vector<Layer*>& layers,
        SharedUniforms& sharedUniforms,
        std::vector<Resource*>& resources
    );
    static void renderLayersTabBarGui
    (
        std::vector<Layer*>& layers,
        SharedUniforms& sharedUniforms,
        std::vector<Resource*>& resources
    );

    static void resetSharedSourceEditor();

    static void renderShaderLanguangeExtensionsMenuGui
    (
        const std::vector<Layer*>& layers,
        SharedUniforms& sharedUniforms
    );

    static void setRenderingTiles
    (
        const std::vector<Layer*>& layers, 
        unsigned int nTiles
    );

    const std::string& name() const {return gui_.name;}
    const glm::ivec2& resolution() const {return resolution_;}
    unsigned long int size() const
    {
        return ((unsigned long int)resolution_.x)*((unsigned long int)resolution_.y);
    }
    float aspectRatio() const {return aspectRatio_;}
    bool isAspectRatioBoundToWindow() const {return flags_.isAspectRatioBoundToWindow;}
    Rendering::Target renderingTarget() const {return rendering_.target;}
    ExportData& exportData() {return exportData_;}

    bool operator==(const Layer& layer){return id_ == layer.id_;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}