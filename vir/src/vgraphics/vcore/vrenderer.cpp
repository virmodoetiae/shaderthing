#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglrenderer.h"

// This bit of code ensures that the most powerful GPU on the system (if there
// are more than one) is used, at least on Windows. On other systems, I have
// no clue, but I suspect it cannot be set from ease from the C++ side of
// things
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include "windows.h"
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace vir
{

Renderer::~Renderer()
{
    delete api_;
    api_ = nullptr;
}

Renderer* Renderer::initialize()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return GlobalPtr<Renderer>::instance(new OpenGLRenderer());
    }
    return nullptr;
}

void Renderer::submit
(
    GeometricPrimitive& geometricPrimitive, 
    Shader* shader, 
    Framebuffer* target, 
    bool clearTarget
)
{
    // If I provide a target framebuffer, set it as the rendering target 
    // by binding it
    auto window = GlobalPtr<Window>::instance();
    bool updateTargetMipmap = false;
    if (target != nullptr)
    {
        window->setViewport(target->width(), target->height());
        target->bind();
        auto minFilterMode(target->colorBufferMinFilterMode());
        if 
        (
            minFilterMode != TextureBuffer::FilterMode::Nearest &&
            minFilterMode != TextureBuffer::FilterMode::Linear
        )
            updateTargetMipmap = true;
    }
    // Otherwise (i.e. no target framebuffer provided), I want to render to the 
    // screen so check if there is any active framebuffer and unbind it
    else 
    {
        window->setViewport(window->width(), window->height());
        if (Framebuffer::activeOne() != nullptr)
            Framebuffer::activeOne()->unbind();
    }
    if (clearTarget)
        api_->clear();
    
    // Render
    submit
    (
        geometricPrimitive.vertexArray(), 
        shader
    );
    if (updateTargetMipmap)
        target->updateColorBufferMipmap();
}

}