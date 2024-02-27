#pragma once

#include <string>
#include <vector>

#include "shaderthing-p/include/texteditor.h"

#include "thirdparty/glm/glm.hpp"

#include "vir/include/vir.h"

namespace ShaderThing
{

typedef vir::TextureBuffer::WrapMode   WrapMode;
typedef vir::TextureBuffer::FilterMode FilterMode;

struct Uniform;
class Resource;
class SharedUniforms;
class ObjectIO;

class Layer : vir::Event::Receiver
{
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
               bool              rename                 = false;
               bool              uncompiledChanges      = false;
               bool              windowBoundAspectRatio = true;
        static bool              restartRendering;
    };
private:
    const uint32_t               id_;
          glm::ivec2             resolution_;
          glm::vec2              resolutionRatio_;
          float                  aspectRatio_;
          float                  depth_;
          std::vector<Uniform*>  uniforms_;
          std::vector<Uniform*>  uncompiledUniforms_;
          Rendering              rendering_;
          GUI                    gui_;
          Flags                  flags_;

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
        glm::ivec2& resolution,
        const bool windowFrameManuallyDragged,
        const bool tryEnfoceWindowAspectRatio=false
    );
    void setDepth(const float depth);
    void setFramebufferWrapMode(int index, WrapMode mode);
    void setFramebufferMagFilterMode(FilterMode mode);
    void setFramebufferMinFilterMode(FilterMode mode);
    void rebuildFramebuffers
    (
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    );
    void save(ObjectIO& io) const;

public:

    static constexpr unsigned int nMaxLayers = 32;

    Layer
    (
        const std::vector<Layer*>& layers,
        const SharedUniforms& sharedUniforms
    );
    ~Layer();
    
    static void save(const std::vector<Layer*>& layers, ObjectIO& io);

    DECLARE_RECEIVABLE_EVENTS(vir::Event::Type::WindowResize)
    void onReceive(vir::Event::WindowResizeEvent& event) override;

    bool removeResourceFromUniforms(const Resource* resource);
    
    bool compileShader(const SharedUniforms& sharedUniforms);
    void renderShader
    (
        vir::Framebuffer* target, 
        const bool clearTarget, 
        const SharedUniforms& sharedUniforms
    );
    void renderTabBarGUI
    (
        SharedUniforms& sharedUnifoms,
        std::vector<Resource*>& resources
    );
    void renderSettingsMenuGUI(std::vector<Resource*>& resources);

    static void renderShaders
    (
        const std::vector<Layer*>& layers,
        vir::Framebuffer* target, 
        const SharedUniforms& sharedUniforms
    );
    static void renderLayersTabBarGUI
    (
        std::vector<Layer*>& layers,
        SharedUniforms& sharedUnifoms,
        std::vector<Resource*>& resources
    );

    bool operator==(const Layer& layer){return id_ == layer.id_;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}