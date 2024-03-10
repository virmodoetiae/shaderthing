#pragma once

#include <string>
#include <vector>

#include "shaderthing-p/include/macros.h"
#include "shaderthing-p/include/texteditor.h"

#include "thirdparty/glm/glm.hpp"

#include "vir/include/vir.h"

namespace ShaderThing
{

typedef vir::TextureBuffer::WrapMode   WrapMode;
typedef vir::TextureBuffer::FilterMode FilterMode;


class ObjectIO;
class PostProcess;
class Resource;
class SharedUniforms;
struct Uniform;

class Layer : vir::Event::Receiver
{
friend PostProcess;
public:
    struct Rendering
    {
        enum class Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
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
               Target            target         = Target::Window;
          FramebufferClearPolicy clearPolicy    = FramebufferClearPolicy::None;
               vir::Quad*        quad           = nullptr;
               vir::Framebuffer* framebufferA   = nullptr;
               vir::Framebuffer* framebufferB   = nullptr;
               vir::Framebuffer* framebuffer    = nullptr;
               vir::Shader*      shader         = nullptr;
       std::vector<PostProcess*> postProcesses  = {};
    };
    struct GUI
    {
               std::string       name;
               std::string       newName;
               std::string       sourceHeader;
               std::string       headerErrors;
               TextEditor        sourceEditor;
        static std::string       defaultSharedSource;
        static TextEditor        sharedSourceEditor;
        
    };
    struct Flags
    {
               bool              rename                     = false;
               bool              pendingDeletion            = false;
               bool              uncompiledChanges          = false;
               bool              isAspectRatioBoundToWindow = true;
        static bool              restartRendering;
    };
    struct Cache
    {
        std::vector<Uniform*>    uncompiledUniforms;
        std::map<
            Uniform*, 
            std::string>         uninitializedResourceFramebuffers;
    };
    struct ExportData
    {
        glm::ivec2               originalResolution;
        glm::ivec2               resolution;
        float                    resolutionScale  = 1.f;
        float                    windowResolutionScale = 1.f;
        bool                     rescaleWithOutput = true;
    };

private:

    const uint32_t               id_;
          glm::ivec2             resolution_;
          glm::vec2              resolutionRatio_;
          float                  aspectRatio_;
          float                  depth_;
          std::vector<Uniform*>  uniforms_;
          Rendering              rendering_;
          GUI                    gui_;
          Flags                  flags_;
          Cache                  cache_;
          ExportData             exportData_;

    //------------------------------------------------------------------------//

    static const unsigned int findFreeId(const std::vector<Layer*>& layers);
    static const std::string& glslVersionSource();
    static const std::string& vertexShaderSource
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
        const SharedUniforms& sharedUniforms
    );
    ~Layer();
    
    static void save(const std::vector<Layer*>& layers, ObjectIO& io);
    static void loadAll
    (
        const ObjectIO& io,
        std::vector<Layer*>& layers,
        const SharedUniforms& sharedUniforms,
        std::vector<Resource*>& resources
    );

    DECLARE_RECEIVABLE_EVENTS(vir::Event::Type::WindowResize)
    void onReceive(vir::Event::WindowResizeEvent& event) override;

    void prepareForExport();
    void resetAfterExport();
    bool removeResourceFromUniforms(const Resource* resource);
    
    bool compileShader(const SharedUniforms& sharedUniforms);
    void renderShader
    (
        vir::Framebuffer* target, 
        const bool clearTarget, 
        const SharedUniforms& sharedUniforms
    );
    static void renderShaders
    (
        const std::vector<Layer*>& layers,
        vir::Framebuffer* target, 
        SharedUniforms& sharedUniforms,
        const unsigned int nRenderPasses = 1
    );

    void renderSettingsMenuGui(std::vector<Resource*>& resources);
    void renderTabBarGui
    (
        SharedUniforms& sharedUnifoms,
        std::vector<Resource*>& resources
    );
    static void renderLayersTabBarGui
    (
        std::vector<Layer*>& layers,
        SharedUniforms& sharedUnifoms,
        std::vector<Resource*>& resources
    );

    const std::string& name() const {return gui_.name;}
    const glm::ivec2& resolution() const {return resolution_;}
    float aspectRatio() const {return aspectRatio_;}
    bool isAspectRatioBoundToWindow() const {return flags_.isAspectRatioBoundToWindow;}
    Rendering::Target renderingTarget() const {return rendering_.target;}
    ExportData& exportData() {return exportData_;}

    bool operator==(const Layer& layer){return id_ == layer.id_;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}