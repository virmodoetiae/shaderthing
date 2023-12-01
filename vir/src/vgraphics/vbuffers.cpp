#include "vpch.h"
#include "vgraphics/vopengl/vopenglbuffers.h"

namespace vir
{

// Buffer base ---------------------------------------------------------------//

// Texture base --------------------------------------------------------------//

const std::unordered_map<TextureBuffer::InternalFormat, std::string> 
    TextureBuffer::internalFormatToName =
{
    {TextureBuffer::InternalFormat::Undefined, "Undefined"},
    {TextureBuffer::InternalFormat::R_UNI_8, "R uint 8-bit norm."},
    {TextureBuffer::InternalFormat::R_UI_8, "R uint 8-bit"},
    {TextureBuffer::InternalFormat::RG_UNI_8, "RG uint 8-bit norm."},
    {TextureBuffer::InternalFormat::RG_UI_8, "RG uint 8-bit)"},
    {TextureBuffer::InternalFormat::RGB_UNI_8, "RGB uint 8-bit norm."},
    {TextureBuffer::InternalFormat::RGB_UI_8, "RGB uint 8-bit)"},
    {TextureBuffer::InternalFormat::RGBA_UNI_8, "RGBA uint 8-bit norm."},
    {TextureBuffer::InternalFormat::RGBA_UI_8, "RGBA uint 8-bit"},
    {TextureBuffer::InternalFormat::RGBA_SF_32, "RGBA float 32-bit"},
};

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
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLTextureBuffer2D(filepath, internalFormat);
    }
    return nullptr;
}

TextureBuffer2D* TextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
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
                internalFormat
            );
    }
    return nullptr;
}

// AnimatedTexture2D ---------------------------------------------------------//

AnimatedTextureBuffer2D::AnimatedTextureBuffer2D() : 
TextureBuffer2D(),
currentFrameIndex_(0),
currentFrame_(nullptr),
frames_(0),
isFrameOwner_(true),
frameDuration_(1.0f/60)
{}

AnimatedTextureBuffer2D::AnimatedTextureBuffer2D
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nFrames,
    InternalFormat internalFormat
) :
TextureBuffer2D(data, width, height, internalFormat),
currentFrameIndex_(0),
currentFrame_(nullptr),
frames_(0),
isFrameOwner_(true),
frameDuration_(1.0f/60)
{
    frames_.resize(nFrames);
}

AnimatedTextureBuffer2D::AnimatedTextureBuffer2D
(
    std::vector<TextureBuffer2D*>& frames,
    bool gainFrameOwnership
) :
TextureBuffer2D(),
currentFrameIndex_(0),
currentFrame_(nullptr),
frames_(frames),
isFrameOwner_(gainFrameOwnership),
frameDuration_(1.0f/60)
{
    if (frames.size() == 0)
        throw std::runtime_error(
R"(vbuffers.cpp - AnimatedTextureBuffer2D(std::vector<TextureBuffer2D*>&, bool) 
- Cannot construct from empty array of frames)"
        );
    // Ensure consistency between frames
    bool firstFrame = true;
    uint32_t width, height;
    InternalFormat internalFormat;
    for (auto* frame : frames_)
    {
        if (firstFrame)
        {
            width = frame->width();
            height = frame->height();
            internalFormat = frame->internalFormat();
            firstFrame = false;
        }
        else if 
        (
            frame->width() != width || 
            frame->height() != height || 
            frame->internalFormat() != internalFormat
        )
        {
            throw std::runtime_error(
R"(vbuffers.cpp - AnimatedTextureBuffer2D(std::vector<TextureBuffer2D*>&, bool) 
- Cannot construct because not all frames have same width, height or internal 
format)"
        );
        }
    }
    currentFrame_ = frames_[currentFrameIndex_];
}

AnimatedTextureBuffer2D::~AnimatedTextureBuffer2D()
{
    if (!isFrameOwner_)
        return;
    for (auto* frame : frames_)
        delete frame;
    frames_.resize(0);
}

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
(
    std::string filepath, 
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLAnimatedTextureBuffer2D(filepath, internalFormat);
    }
    return nullptr;
}

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nFrames,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLAnimatedTextureBuffer2D
            (
                data, 
                width, 
                height,
                nFrames,
                internalFormat
            );
    }
    return nullptr;
}

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
(
    std::vector<TextureBuffer2D*>& frames,
    bool gainFrameOwnership
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLAnimatedTextureBuffer2D
            (
                frames, gainFrameOwnership
            );
    }
    return nullptr;
}

TextureBuffer2D* AnimatedTextureBuffer2D::nextFrame() 
{
    if (frames_.size() == 0)
        return nullptr;
    ++currentFrameIndex_;
    currentFrameIndex_ %= frames_.size();
    currentFrame_ = frames_[currentFrameIndex_];
    return currentFrame_;
}

int AnimatedTextureBuffer2D::currentFrameId() const
{
    if (currentFrame_ == nullptr)
        return -1;
    return currentFrame_->id();
}

void AnimatedTextureBuffer2D::setFrameIndex(uint32_t index)
{
    if (frames_.size() == 0)
        return;
    currentFrameIndex_ = index % frames_.size();
    currentFrame_ = frames_[currentFrameIndex_];
}

void AnimatedTextureBuffer2D::setFrameIndexFromTime(float time)
{
    if (frames_.size() == 0)
        return;
    currentFrameIndex_ = int(time/frameDuration_) % frames_.size();
    currentFrame_ = frames_[currentFrameIndex_];
}

// CubeMap -------------------------------------------------------------------//

CubeMapBuffer* CubeMapBuffer::create
(
    std::string filepaths[6], 
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLCubeMapBuffer(filepaths, internalFormat);
    }
    return nullptr;
}

CubeMapBuffer* CubeMapBuffer::create
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
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
                internalFormat
            );
    }
    return nullptr;
}

// Framebuffer ---------------------------------------------------------------//

Framebuffer* Framebuffer::activeOne_ = nullptr;

Framebuffer* Framebuffer::create
(
    uint32_t width, 
    uint32_t height,
    TextureBuffer::InternalFormat format
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLFramebuffer(width, height, format);
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