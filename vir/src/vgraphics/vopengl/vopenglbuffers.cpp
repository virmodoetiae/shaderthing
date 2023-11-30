#include "vpch.h"
#include "vgraphics/vopengl/vopenglbuffers.h"
#include "thirdparty/stb/stb_image.h"

namespace vir
{

GLint OpenGLInternalFormat(TextureBuffer::InternalFormat internalFormat)
{
    switch (internalFormat)
    {
        case TextureBuffer::InternalFormat::R_UNI_8 :
            return GL_R8;
        case TextureBuffer::InternalFormat::R_UI_8 :
            return GL_R8UI;
        case TextureBuffer::InternalFormat::RG_UNI_8 :
            return GL_RG8;
        case TextureBuffer::InternalFormat::RG_UI_8 :
            return GL_RG8UI;
        case TextureBuffer::InternalFormat::RGB_UNI_8 :
            return GL_RGB8;
        case TextureBuffer::InternalFormat::RGB_UI_8 :
            return GL_RGB8UI;
        case TextureBuffer::InternalFormat::RGBA_UNI_8 :
            return GL_RGBA8;
        case TextureBuffer::InternalFormat::RGBA_UI_8 :
            return GL_RGBA8UI;
        case TextureBuffer::InternalFormat::RGBA_SF_32 :
            return GL_RGBA32F;
    }
    return 0;
}

GLint OpenGLFormat(TextureBuffer::InternalFormat internalFormat)
{
    switch (internalFormat)
    {
        case TextureBuffer::InternalFormat::R_UNI_8 :
            return GL_RED;
        case TextureBuffer::InternalFormat::R_UI_8 :
            return GL_RED;
        case TextureBuffer::InternalFormat::RG_UNI_8 :
            return GL_RG;
        case TextureBuffer::InternalFormat::RG_UI_8 :
            return GL_RG;
        case TextureBuffer::InternalFormat::RGB_UNI_8 :
            return GL_RGB;
        case TextureBuffer::InternalFormat::RGB_UI_8 :
            return GL_RGB;
        case TextureBuffer::InternalFormat::RGBA_UNI_8 :
            return GL_RGBA;
        case TextureBuffer::InternalFormat::RGBA_UI_8 :
            return GL_RGBA;
        case TextureBuffer::InternalFormat::RGBA_SF_32 :
            return GL_RGBA;
    }
    return 0;
}

GLint OpenGLType(TextureBuffer::InternalFormat internalFormat)
{
    switch (internalFormat)
    {
        case TextureBuffer::InternalFormat::R_UNI_8 :
        case TextureBuffer::InternalFormat::R_UI_8 :
        case TextureBuffer::InternalFormat::RG_UNI_8 :
        case TextureBuffer::InternalFormat::RG_UI_8 :
        case TextureBuffer::InternalFormat::RGB_UNI_8 :
        case TextureBuffer::InternalFormat::RGB_UI_8 :
        case TextureBuffer::InternalFormat::RGBA_UNI_8 :
        case TextureBuffer::InternalFormat::RGBA_UI_8 :
            return GL_UNSIGNED_BYTE;    // Not sure if the UIs should be 
                                        // GL_UNSIGNED_INT instead
        case TextureBuffer::InternalFormat::RGBA_SF_32 :
            return GL_FLOAT;
    }
    return 0;
}

const std::unordered_map<TextureBuffer::WrapMode, GLint> wrapModeToGLint_ = 
{
    {TextureBuffer::WrapMode::ClampToBorder, GL_CLAMP_TO_BORDER},
    {TextureBuffer::WrapMode::ClampToEdge, GL_CLAMP_TO_EDGE},
    {TextureBuffer::WrapMode::Repeat, GL_REPEAT},
    {TextureBuffer::WrapMode::MirroredRepeat, GL_MIRRORED_REPEAT}
};

const std::unordered_map<uint32_t, GLint> wrapIndexToGLint_ = 
{
    {0, GL_TEXTURE_WRAP_S},
    {1, GL_TEXTURE_WRAP_T},
    {2, GL_TEXTURE_WRAP_R}
};

const std::unordered_map<TextureBuffer::FilterMode, GLint> filterModeToGLint_ = 
{
    {TextureBuffer::FilterMode::Nearest, GL_NEAREST},
    {TextureBuffer::FilterMode::Linear, GL_LINEAR},
    {TextureBuffer::FilterMode::NearestMipmapNearest,GL_NEAREST_MIPMAP_NEAREST},
    {TextureBuffer::FilterMode::NearestMipmapLinear, GL_NEAREST_MIPMAP_LINEAR},
    {TextureBuffer::FilterMode::LinearMipmapNearest, GL_LINEAR_MIPMAP_NEAREST},
    {TextureBuffer::FilterMode::LinearMipmapLinear, GL_LINEAR_MIPMAP_LINEAR}
};

//----------------------------------------------------------------------------//
// Texture2D buffer ----------------------------------------------------------//
//----------------------------------------------------------------------------//

void OpenGLTextureBuffer2D::initialize
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    if (internalFormat == InternalFormat::Undefined)
        throw std::runtime_error
        (
R"(vopenglbuffers.cpp - OpenGLTextureBuffer2D::initialize(const unsigned char*,
uint32_t, uint32_t, InternalFormat) - Cannot create a texture from the provided
data if the internal format is undefined)"
        );
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    for (int i=0;i<2;i++)
        glTexParameteri
        (
            GL_TEXTURE_2D, 
            wrapIndexToGLint_.at(i), 
            wrapModeToGLint_.at(wrapModes_[i])
        );
    // Zoom in filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(magFilterMode_));
    // Zoom out filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(minFilterMode_));

    // Create texture
    GLint glFormat = OpenGLFormat(internalFormat);
    if (glFormat != GL_RGBA)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    else
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 4 Is default
    GLint glInternalFormat = OpenGLInternalFormat(internalFormat);
    glTexImage2D
    (
        GL_TEXTURE_2D, 
        0, 
        glInternalFormat, 
        width, 
        height, 
        0, 
        glFormat, 
        OpenGLType(internalFormat),
        data
    );
    
    // Swizzling setting
    if (nChannels_ == 1) 
    {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    else if (nChannels_ == 2) 
    {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    else if (nChannels_ == 3)
    {
        GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    width_ = width;
    height_ = height;
    internalFormat_ = internalFormat;
    nChannels_ = TextureBuffer::nChannels(internalFormat);
}

OpenGLTextureBuffer2D::OpenGLTextureBuffer2D
(
    std::string filepath, 
    InternalFormat internalFormat
)
{
    // Load textue data from file
    stbi_set_flip_vertically_on_load(true);
    int width, height, nChannels;
    int requestedNChannels = TextureBuffer::nChannels(internalFormat);
    unsigned char* data = stbi_load
    (
        filepath.c_str(), 
        &width, 
        &height, 
        &nChannels, 
        requestedNChannels
    );
    if (!data)
    {   
        throw std::runtime_error
        (
R"(vopenglbuffers.cpp - OpenGLTextureBuffer2D(std::string, InternalFormat) -
failed to load data from file)"
        );
    }
    nChannels_ = requestedNChannels ? requestedNChannels : (uint32_t)nChannels;
    if (internalFormat == InternalFormat::Undefined)
        internalFormat = defaultInternalFormat(nChannels_);
    // Construct actual GL object from data
    initialize 
    (
        data,
        width,
        height,
        internalFormat
    );
    stbi_image_free(data);
    data = nullptr;
}

OpenGLTextureBuffer2D::OpenGLTextureBuffer2D
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
) : 
TextureBuffer2D(data, width, height, internalFormat)
{
    initialize // Construct actual GL object from data
    (
        data,
        width,
        height,
        internalFormat
    );
}

OpenGLTextureBuffer2D::~OpenGLTextureBuffer2D()
{
    glDeleteTextures(1, &id_);
}

void OpenGLTextureBuffer2D::setWrapMode
(
    uint32_t index,
    TextureBuffer::WrapMode mode
)
{
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri
    (
        GL_TEXTURE_2D, 
        wrapIndexToGLint_.at(index), 
        wrapModeToGLint_.at(mode)
    );
    wrapModes_[index] = mode;
}

void OpenGLTextureBuffer2D::setMagFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri
    (
        GL_TEXTURE_2D, 
        GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(mode)
    );
    magFilterMode_ = mode;
}

void OpenGLTextureBuffer2D::setMinFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri
    (
        GL_TEXTURE_2D, 
        GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(mode)
    );
    minFilterMode_ = mode;
}

void OpenGLTextureBuffer2D::bind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void OpenGLTextureBuffer2D::unbind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//----------------------------------------------------------------------------//
// CubeMap buffer ------------------------------------------------------------//
//----------------------------------------------------------------------------//

void OpenGLCubeMapBuffer::initialize
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    if (internalFormat == InternalFormat::Undefined)
        throw std::runtime_error
        (
R"(vopenglbuffers.cpp - OpenGLCubeMapBuffer(const unsigned char**, uint32_t, 
uint32_t, InternalFormat) - cannot create a cubemap from the provided data if
the internal format is undefined)"
        );
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    GLint glFormat = OpenGLFormat(internalFormat);
    if (glFormat != GL_RGBA)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    else
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 4 Is default
    GLint glInternalFormat = OpenGLInternalFormat(internalFormat);
    for (unsigned int i = 0; i < 6; i++)
        glTexImage2D
        (
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
            0, 
            glInternalFormat, 
            width, 
            height, 
            0, 
            glFormat, 
            OpenGLType(internalFormat), 
            faceData[i]
        );
    // Zoom in filter
    magFilterMode_ = FilterMode::Linear;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(magFilterMode_));
    // Zoom out filter
    minFilterMode_ = FilterMode::Linear;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(minFilterMode_));
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, borderColor);
    for (int i=0;i<3;i++)
    {
        wrapModes_[i] = WrapMode::ClampToEdge;
        glTexParameteri
        (
            GL_TEXTURE_CUBE_MAP, 
            wrapIndexToGLint_.at(i), 
            wrapModeToGLint_.at(wrapModes_[i])
        );
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    width_ = width;
    height_ = height;
    nChannels_ = TextureBuffer::nChannels(internalFormat);
}

OpenGLCubeMapBuffer::OpenGLCubeMapBuffer
(
    std::string filepaths[6], 
    InternalFormat internalFormat
)
{
    int requestedNChannels = TextureBuffer::nChannels(internalFormat);
    const unsigned char* faceData[6];
    stbi_set_flip_vertically_on_load(false);
    for (int i=0; i<6; i++)
    {
        int width, height, nChannels;
        faceData[i] = stbi_load
        (
            filepaths[i].c_str(), 
            &width, 
            &height, 
            &nChannels, 
            requestedNChannels
        );
        if (i == 0)
        {
            width_ = (uint32_t)width;
            height_ = (uint32_t)height;
            nChannels_ = 
                requestedNChannels ? requestedNChannels : (uint32_t)nChannels;
            if (internalFormat == InternalFormat::Undefined)
                internalFormat == 
                    TextureBuffer::defaultInternalFormat(nChannels_);
            internalFormat_ = internalFormat;
        }
        else if 
        (
            width != width_ || 
            height != height_ || 
            (requestedNChannels==0 && nChannels != nChannels_) || 
            !faceData[i]
        )
        {
            for (int j=0; j<i; j++)
                stbi_image_free((void*)faceData[j]);
            throw std::runtime_error
            (
R"(vopenglbuffers.cpp - OpenGLCubeMapBuffer(std::string[6], InternalFormat),
failed to construct due to inconsistent face width, height or internalFormat)"
            );
        }
    }
    initialize(faceData, width_, height_, internalFormat_);
    for (int i=0; i<6; i++)
        stbi_image_free((void*)faceData[i]);
}

OpenGLCubeMapBuffer::OpenGLCubeMapBuffer
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    initialize(faceData, width, height, internalFormat);
}

OpenGLCubeMapBuffer::~OpenGLCubeMapBuffer()
{
    glDeleteTextures(1, &id_);
}

void OpenGLCubeMapBuffer::bind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
}

void OpenGLCubeMapBuffer::unbind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLCubeMapBuffer::setWrapMode
(
    uint32_t index,
    TextureBuffer::WrapMode mode
)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);    
    glTexParameteri
    (
        GL_TEXTURE_CUBE_MAP, 
        wrapIndexToGLint_.at(index), 
        wrapModeToGLint_.at(mode)
    );
    wrapModes_[index] = mode;
}

void OpenGLCubeMapBuffer::setMagFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    glTexParameteri
    (
        GL_TEXTURE_CUBE_MAP, 
        GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(mode)
    );
    magFilterMode_ = mode;
}

void OpenGLCubeMapBuffer::setMinFilterMode

(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    glTexParameteri
    (
        GL_TEXTURE_CUBE_MAP, 
        GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(mode)
    );
    minFilterMode_ = mode;
}

//----------------------------------------------------------------------------//
// Frame buffer --------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLFramebuffer::OpenGLFramebuffer
(
    uint32_t width, 
    uint32_t height,
    TextureBuffer::InternalFormat internalFormat
)
{
    width_ = width;
    height_ = height;

    // Create framebuffer
    glGenFramebuffers(1, &id_);
    glBindFramebuffer(GL_FRAMEBUFFER, id_);

    colorBuffer_ = new OpenGLTextureBuffer2D
    (
        NULL, 
        width, 
        height, 
        internalFormat
    );

    colorBufferId_ = colorBuffer_->id();
    colorBuffer_->setMinFilterMode(TextureBuffer::FilterMode::Linear);
    glFramebufferTexture2D
    (
        GL_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0, 
        GL_TEXTURE_2D, 
        colorBufferId_, 
        0
    );

    glGenRenderbuffers(1, &depthBufferId_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferId_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer
    (
        GL_FRAMEBUFFER, 
        GL_DEPTH_STENCIL_ATTACHMENT, 
        GL_RENDERBUFFER, 
        depthBufferId_
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer setup invalid or incomplete");

    // All color attachments to which I can (but don't have to) draw
    //GLenum bufs[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, 
    //GL_COLOR_ATTACHMENT2};
    //glDrawBuffers(3, bufs);
    
    GLenum buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
    if (activeOne_ == this)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        activeOne_ = nullptr;
    }
    glDeleteRenderbuffers(1, &depthBufferId_);
    //glDeleteTextures(1, &colorBufferId_);
    delete colorBuffer_;
    glDeleteFramebuffers(1, &id_);
}

void OpenGLFramebuffer::bind()
{
    if (activeOne_ != nullptr)
    {
        if (activeOne_ == this)
            return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
    activeOne_ = this;
}

void OpenGLFramebuffer::unbind()
{
    if (activeOne_ == nullptr)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    activeOne_ = nullptr;
}

void OpenGLFramebuffer::bindColorBuffer(uint32_t unit)
{
    colorBuffer_->bind(unit);
}

void OpenGLFramebuffer::bindDepthBuffer(uint32_t unit)
{
    /*glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, depthBufferId_);*/
    
    //glBindRenderbuffer(GL_RENDERBUFFER, depthBufferId_);
}

void OpenGLFramebuffer::colorBufferData(unsigned char* data, bool yFlip)
{
    bind();
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, data);
    if (yFlip)
        for(int line = 0; line != height_/2; ++line) 
        {
            std::swap_ranges
            (
                &data[0] + 4 * width_ * line,
                &data[0] + 4 * width_ * (line+1),
                &data[0] + 4 * width_ * (height_-line-1)
            );
        }
}

void OpenGLFramebuffer::clearColorBuffer(float r, float g, float b, float a)
{
    auto* previouslyActiveFramebuffer = activeOne_;
    bind();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    if (previouslyActiveFramebuffer != nullptr)
        previouslyActiveFramebuffer->bind();
}

//----------------------------------------------------------------------------//
// Vertex buffer -------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
{
    count_ = size/((uint32_t)sizeof(float));
    glGenBuffers(1, &id_);
    glBindBuffer(GL_ARRAY_BUFFER, id_);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindVertexBuffer(this);
    }
    glDeleteBuffers(1, &id_);
}

void OpenGLVertexBuffer::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, id_);
    VertexBuffer::setLayout();
}

void OpenGLVertexBuffer::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLVertexBuffer::setLayout
(
    const VertexBufferLayout& layout
)
{
    auto GLType = [](const VertexBufferLayout::Element& e)
    {
        switch(e.variable.type)
        {
            case (Shader::Variable::Type::Bool) :
                return GL_BOOL;
            case (Shader::Variable::Type::Int) :
            case (Shader::Variable::Type::Int2) :
            case (Shader::Variable::Type::Int3) :
                return GL_INT;
            case (Shader::Variable::Type::Float) :
            case (Shader::Variable::Type::Float2) :
            case (Shader::Variable::Type::Float3) :
            case (Shader::Variable::Type::Float4) :
            case (Shader::Variable::Type::Mat3) :
            case (Shader::Variable::Type::Mat4) :
                return GL_FLOAT;
        }
        return GL_FLOAT;
    };

    uint32_t stride(layout.stride());
    for(const VertexBufferLayout::Element& e : layout.elements())
    {
        glVertexAttribPointer
        (   
            e.location, 
            e.variable.nCmpts, 
            GLType(e), 
            e.normalized? GL_TRUE : GL_FALSE, 
            stride, 
            (void*)(intptr_t)e.offset
        );
        glEnableVertexAttribArray(e.location);
    }
}

void OpenGLVertexBuffer::updateVertices(float* vertices, uint32_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, id_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}

//----------------------------------------------------------------------------//
// Index buffer --------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t size)
{
    count_ = size/((uint32_t)sizeof(uint32_t));
    glGenBuffers(1, &id_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindIndexBuffer();
    }
    glDeleteBuffers(1, &id_);
}

void OpenGLIndexBuffer::bind() 
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
}

void OpenGLIndexBuffer::unbind() 
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//----------------------------------------------------------------------------//
// Vertex array --------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLVertexArray::OpenGLVertexArray()
{
    glBindVertexArray(0);
    glGenVertexArrays(1, &id_);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
    for (auto& v : vertexBuffers_)
    {
        unbindVertexBuffer(v);
    }
    unbindIndexBuffer();
    glDeleteVertexArrays(1, &id_);
}

void OpenGLVertexArray::bind()
{
    if (activeOne_ != nullptr)
    {
        if (activeOne_ == this)
            return;
    }
    glBindVertexArray(id_);
    activeOne_ = this;
}

void OpenGLVertexArray::unbind()
{
    if (activeOne_ == nullptr)
        return;
    glBindVertexArray(0);
    activeOne_ = nullptr;
}

void OpenGLVertexArray::bindVertexBuffer(VertexBuffer* vertexBuffer)
{
    // When bidning a vertex buffer to this vertex array, first check if the 
    // buffer was already registered. If not, register it and also register the
    // array with the buffer
    auto it = Helpers::findInPtrVector(vertexBuffer, vertexBuffers_);
    if (it != vertexBuffers_.end()) // If found
        return;
    vertexBuffers_.push_back(vertexBuffer);
    vertexBuffer->vertexArrays().push_back(this);
    
    bind();
    vertexBuffer->bind();
}

void OpenGLVertexArray::unbindVertexBuffer(VertexBuffer* vertexBuffer)
{
    // When unregistering a vertex buffer from this array, first check if the
    // buffer was already registered. If so, unregister it, and also unregister
    // the array with the buffer
    auto it = Helpers::findInPtrVector(vertexBuffer, vertexBuffers_);
    if (it == vertexBuffers_.end()) // If not found
        return;
    vertexBuffers_.erase(it);
    auto it2 = Helpers::findInPtrVector<VertexArray>
    (
        this, vertexBuffer->vertexArrays()
    );
    if (it2 != vertexBuffer->vertexArrays().end()) // If found
        vertexBuffer->vertexArrays().erase(it2);
    
    bind();
    vertexBuffer->unbind();
}

void OpenGLVertexArray::bindIndexBuffer(IndexBuffer* indexBuffer)
{
    if (indexBuffer == nullptr)
        return;
    indexBuffer->vertexArrays().push_back(this);
    bind();
    indexBuffer->bind();
    indexBuffer_ = indexBuffer;
}

void OpenGLVertexArray::unbindIndexBuffer()
{
    if (indexBuffer_ == nullptr)
        return;
    auto it = Helpers::findInPtrVector<VertexArray>
    (
        this, indexBuffer_->vertexArrays()
    );
    if (it != indexBuffer_->vertexArrays().end()) // If found
        indexBuffer_->vertexArrays().erase(it);
    bind();
    indexBuffer_->unbind();
    indexBuffer_ = nullptr;
}

}