#pragma once

#include <string>
#include <vector>

#include "thirdparty/glm/glm.hpp"
#include "thirdparty/imguitexteditor/imguitexteditor.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

struct Uniform;
class SharedUniforms;

class Layer : vir::Event::Receiver
{
private:
    struct Rendering
    {
        enum struct Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
               Target                target         = Target::Window;
               vir::Quad*            quad           = nullptr;
               vir::Framebuffer*     framebufferA   = nullptr;
               vir::Framebuffer*     framebufferB   = nullptr;
               vir::Framebuffer*     framebuffer    = nullptr;
               vir::Shader*          shader         = nullptr;
    };
    struct GUI
    {
               std::string           name;
               std::string           newName;
               std::string           sourceHeader;
               ImGuiExtd::TextEditor sourceEditor;
        static ImGuiExtd::TextEditor sharedSourceEditor;
    };
    struct Flags
    {
               bool                  rename               = false;
               bool                  errorsInSource       = false;
               bool                  errorsInSourceHeader = false;
               bool                  uncompiledChanges    = false;
        static bool                  errorsInSharedSource;
    };

    const uint32_t                   id_;
          glm::ivec2                 resolution_;
          float                      aspectRatio_;
          float                      depth_;
          std::vector<Uniform*>      uniforms_;
          std::vector<Uniform*>      uncompiledUniforms_;
          Rendering                  rendering_;
          GUI                        gui_;
          Flags                      flags_;

    //------------------------------------------------------------------------//

    static const unsigned int findFreeId(const std::vector<Layer*>& layers);
    static const std::string& glslVersionSource();
    static const std::string& vertexShaderSource
    (
        const SharedUniforms& sharedUniforms
    );
    std::tuple<std::string, unsigned int> fragmentShaderHeaderSourceAndLineCount
    (
        const SharedUniforms& sharedUniforms
    ) const;

public:

    Layer
    (
        const std::vector<Layer*>& layers,
        const SharedUniforms& sharedUniforms
    );
    ~Layer();

    DECLARE_RECEIVABLE_EVENTS(vir::Event::Type::WindowResize)
    void onReceive(vir::Event::WindowResizeEvent& event) override;
    
    bool compileShader(const SharedUniforms& sharedUniforms);
    void renderShader
    (
        vir::Framebuffer* target, 
        const bool clearTarget, 
        const SharedUniforms& sharedUniforms
    );

    void renderSettingsMenuGUI();
    static void renderLayersTabBarGUI
    (
        std::vector<Layer*>& layers,
        const SharedUniforms& sharedUnifoms
    );
    void renderTabBarGUI();

    bool operator==(const Layer& layer){return id_ == layer.id_;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}