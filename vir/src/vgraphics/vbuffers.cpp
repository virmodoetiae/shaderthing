#include "vpch.h"
#include "vgraphics/vopengl/vopenglbuffers.h"

namespace vir
{



// Buffer base ---------------------------------------------------------------//

// Texture base --------------------------------------------------------------//

const std::unordered_map<TextureBuffer::WrapMode, std::string> 
    TextureBuffer::wrapModeToName =
{
    {TextureBuffer::WrapMode::MirroredRepeat, "Mirror & repeat"},
    {TextureBuffer::WrapMode::Repeat, "Repeat"},
    {TextureBuffer::WrapMode::ClampToEdge, "Clamp to edge"},
    {TextureBuffer::WrapMode::ClampToBorder, "Clamp to border"}
};

const std::unordered_map<TextureBuffer::FilterMode, std::string> 
    TextureBuffer::filterModeToName =
{
    {TextureBuffer::FilterMode::LinearMipmapLinear, "Linear, mipmap-linear"},
    {TextureBuffer::FilterMode::NearestMipmapLinear, "Nearest, mipmap-linear"},
    {TextureBuffer::FilterMode::LinearMipmapNearest, "Linear, mipmap-nearest"},
    {TextureBuffer::FilterMode::NearestMipmapNearest,"Nearest, mipmap-nearest"},
    {TextureBuffer::FilterMode::Linear, "Linear"},
    {TextureBuffer::FilterMode::Nearest, "Nearest"}
};

// Texture2D -----------------------------------------------------------------//

TextureBuffer2D* TextureBuffer2D::create
(
    std::string filepath, 
    uint32_t requestedChannels
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLTextureBuffer2D(filepath, requestedChannels);
    }
    return nullptr;
}

TextureBuffer2D* TextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nChannels
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLTextureBuffer2D
            (
                data, 
                width, 
                height,
                nChannels
            );
    }
    return nullptr;
}

// CubeMap -------------------------------------------------------------------//

CubeMapBuffer* CubeMapBuffer::create
(
    std::string filepaths[6], 
    uint32_t requestedChannels
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLCubeMapBuffer(filepaths, requestedChannels);
    }
    return nullptr;
}

CubeMapBuffer* CubeMapBuffer::create
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    uint32_t nChannels
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLCubeMapBuffer
            (
                faceData, 
                width, 
                height,
                nChannels
            );
    }
    return nullptr;
}

// Framebuffer ---------------------------------------------------------------//

Framebuffer* Framebuffer::activeOne_ = nullptr;

Framebuffer* Framebuffer::create(uint32_t width, uint32_t height)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLFramebuffer(width, height);
    }
    return nullptr;
}

// Vertex Buffer layout ------------------------------------------------------//

VertexBufferLayout::VertexBufferLayout
(
    const std::initializer_list<VertexBufferLayout::Element>& elements
) :
stride_(0),
elements_(elements)
{
    int location = 0;
    for (Element& e : elements_)
    {
        e.offset = stride_;
        e.location = location;
        stride_ += e.variable.size;
        location += 1;
    }
}

// Vertex Buffer -------------------------------------------------------------//

VertexBuffer* VertexBuffer::create(float* vertices, uint32_t size)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLVertexBuffer(vertices, size);
    }
    return nullptr;
}

VertexBuffer::~VertexBuffer()
{
    if (layout_ != nullptr)
        delete layout_;
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindVertexBuffer(this);
    }
}

void VertexBuffer::setLayout
(
    const std::initializer_list<VertexBufferLayout::Element>& elements
)
{
    layout_ = new VertexBufferLayout(elements);
    setLayout(*layout_);
}

void VertexBuffer::setLayout()
{
    if (layout_ != nullptr)
        setLayout(*layout_);
}

// Index buffer --------------------------------------------------------------//

IndexBuffer* IndexBuffer::create(uint32_t* indices, uint32_t size)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLIndexBuffer(indices, size);
    }
    return nullptr;
}

IndexBuffer::~IndexBuffer()
{
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindIndexBuffer();
    }
}

// Vertex array --------------------------------------------------------------//

VertexArray* VertexArray::activeOne_ = nullptr;

VertexArray* VertexArray::create()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLVertexArray();
    }
    return nullptr;
}

}