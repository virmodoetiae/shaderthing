#pragma once

#include <string>
#include <vector>

#include "thirdparty/glm/glm.hpp"
#include "thirdparty/imguitexteditor/imguitexteditor.h"

namespace vir
{
    class Quad;
    class Framebuffer;
    class Shader;
}

namespace ShaderThing
{

struct Uniform;

struct Layer
{
    struct Rendering
    {
        enum struct Target
        {
            Window,
            InternalFramebuffer,
            InternalFramebufferAndWindow
        };
               Target                       target         = Target::Window;
               vir::Quad*                   quad           = nullptr;
               bool                         flippedBuffers = false;
               vir::Framebuffer*            framebufferA   = nullptr;
               vir::Framebuffer*            framebufferB   = nullptr;
               vir::Framebuffer*            framebuffer    = nullptr;
               bool                         compileShader  = true;
               vir::Shader*                 shader         = nullptr;
    };
    struct GUI
    {
               bool                         rename               = false;
               std::string                  name;
               std::string                  newName;
               std::string                  sourceHeader;
               ImGuiExtd::TextEditor        sourceEditor;
               bool                         sourceEditsCompiled  = false;
               bool                         errorsInSourceHeader = false;
               bool                         errorsInSource       = false;
        static ImGuiExtd::TextEditor        sharedSourceEditor;
        static bool                         errorsInSharedSource;
    };

    const uint32_t              id;
          glm::ivec2            resolution;
          float                 aspectRatio;
          float                 depth;
          std::vector<Uniform*> uniforms;
          std::vector<Uniform*> uncompiledUniforms;
          Rendering             rendering;
          GUI                   gui;

    bool operator==(const Layer& layer){return id == layer.id;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}