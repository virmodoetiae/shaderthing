#pragma once

#include <string>
#include <vector>

#include "thirdparty/glm/glm.hpp"
#include "thirdparty/imguitexteditor/imguitexteditor.h"

#include "shaderthing-p/include/data/uniform.h"

namespace vir
{
    class Quad;
    class Framebuffer;
    class Shader;
}

namespace ShaderThing
{

struct Layer
{
    struct Rendering
    {
        vir::Quad*             quad           = nullptr;
        bool                   flippedBuffers = false;
        vir::Framebuffer*      framebufferA   = nullptr;
        vir::Framebuffer*      framebufferB   = nullptr;
        vir::Shader*           shader         = nullptr;
    };
    struct GUI
    {
        bool                   rename         = false;
        std::string            name;
        std::string            newName;
        ImGuiExtd::TextEditor  fragmentSourceEditor;
    };

    const uint32_t             id;
          glm::ivec2           resolution;
          float                aspectRatio;
          float                depth;
          std::string          fragmentSource = 
R"(fragColor = vec4(
    .5+.25*sin(2*(qc+iTime)),
    .75,
    1.);)";
          std::vector<Uniform*> uniforms;
          Rendering             rendering;
          GUI                   gui;

    bool operator==(const Layer& layer){return id == layer.id;}
    bool operator!=(const Layer& layer){return !(*this == layer);}
};

}