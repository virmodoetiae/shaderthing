#include "vpch.h"
#include "vgraphics/vopengl/vopenglrenderer.h"

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
    Framebuffer* framebuffer, 
    bool clearFramebuffer
)
{
    // If I provide a framebuffer, set that as the rendering target by binding
    // it
    auto window = GlobalPtr<Window>::instance();
    if (framebuffer != nullptr)
    {
        window->setViewport(framebuffer->width(), framebuffer->height());
        framebuffer->bind();
        if (clearFramebuffer)
            api_->clear();
    }
    // Otherwise (i.e. no framebuffer provided), I want to render to the screen
    // so check if there is any active framebuffer and unbind it
    else 
    {
        window->setViewport(window->width(), window->height());
        if (Framebuffer::activeOne() != nullptr)
            Framebuffer::activeOne()->unbind();
    }
    // Render
    submit
    (
        geometricPrimitive.vertexArray(), 
        shader
    );
}

}