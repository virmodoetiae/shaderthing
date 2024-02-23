#pragma once

#include <string>
#include <vector>

#include "shaderthing-p/include/texteditor.h"

#include "thirdparty/glm/glm.hpp"

#include "vir/include/vir.h"

namespace ShaderThing
{

struct Uniform;
class Resource;
class SharedUniforms;

class Layer : vir::Event::Receiver
{
public:
    struct Rendering
    {
        enum struct Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
               Target            target         = Target::InternalFramebufferAndWindow;
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
        static TextEditor        sharedSourceEditor;
    };
    struct Flags
    {
               bool              rename               = false;
               bool              uncompiledChanges    = false;
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
        const bool windowFrameManuallyDragged
    );
    void setDepth(const float depth);
    void rebuildFramebuffers
    (
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    );

public:

    static constexpr unsigned int nMaxLayers = 32;

    Layer
    (
        const std::vector<Layer*>& layers,
        const SharedUniforms& sharedUniforms
    );
    ~Layer();

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
        std::vector<Resource*> resources
    );
    void renderSettingsMenuGUI();

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
        std::vector<Resource*> resources
    );

    bool operator==(const Layer& layer){return id_ == layer.id_;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}