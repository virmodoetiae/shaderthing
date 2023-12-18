#ifndef V_OPENGL_BUFFERS_H
#define V_OPENGL_BUFFERS_H

#include <unordered_map>
#include "vgraphics/vcore/vbuffers.h"
#include "thirdparty/glad/include/glad/glad.h"

namespace vir
{

extern const std::unordered_map<TextureBuffer::WrapMode, GLint> 
    wrapModeToGLint_;
extern const std::unordered_map<uint32_t, GLint> wrapIndexToGLint_;
extern const std::unordered_map<TextureBuffer::FilterMode, GLint> 
    filterModeToGLint_;

static GLint OpenGLInternalFormat(TextureBuffer::InternalFormat internalFormat);

static GLint OpenGLFormat(TextureBuffer::InternalFormat internalFormat);

static GLint OpenGLType(TextureBuffer::InternalFormat internalFormat);

class OpenGLTextureBuffer2D : public TextureBuffer2D
{
private:
    void initialize
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
    );
public:
    OpenGLTextureBuffer2D
    (
        std::string filepath, 
        InternalFormat internalFormat=InternalFormat::Undefined
    );
    OpenGLTextureBuffer2D
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
    );
    ~OpenGLTextureBuffer2D();
    void setWrapMode
    (
        uint32_t index,
        WrapMode value
    ) override;
    void setMagFilterMode(FilterMode mode) override;
    void setMinFilterMode(FilterMode mode) override;
    void bind(uint32_t) override;
    void unbind(uint32_t) override;
};

class OpenGLAnimatedTextureBuffer2D : public AnimatedTextureBuffer2D
{
private:
    static uint32_t nextFreeId_; // Global Id counter since id of animation
                                 // container not tied to any TextureBuffer2D 
                                 // instance
    void initialize
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        uint32_t nFrames,
        InternalFormat internalFormat
    );
public:
    OpenGLAnimatedTextureBuffer2D // Construct from .gif filepath
    (
        std::string filepath, 
        InternalFormat internalFormat=InternalFormat::Undefined
    );
    OpenGLAnimatedTextureBuffer2D // Construct from raw data and frame info
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        uint32_t nFrames,
        InternalFormat internalFormat
    );
    OpenGLAnimatedTextureBuffer2D // Construct from existing frames
    (
        std::vector<TextureBuffer2D*>& frames,
        bool gainFrameOwnership = false
    );
    ~OpenGLAnimatedTextureBuffer2D();
    void setWrapMode
    (
        uint32_t index,
        WrapMode value
    ) override;
    void setMagFilterMode(FilterMode mode) override;
    void setMinFilterMode(FilterMode mode) override;
    void bind(uint32_t) override;
    void unbind(uint32_t) override;
};

class OpenGLCubeMapBuffer : public CubeMapBuffer
{
private:
    void initialize
    (
        const unsigned char* faceData[6], 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
    );
public:
    OpenGLCubeMapBuffer
    (
        std::string filepaths[6], 
        InternalFormat internalFormat=InternalFormat::Undefined
    );
    OpenGLCubeMapBuffer
    (
        const unsigned char* faceData[6], 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
    );
    ~OpenGLCubeMapBuffer();
    void setWrapMode
    (
        uint32_t index,
        WrapMode value
    ) override;
    void setMagFilterMode(FilterMode mode) override;
    void setMinFilterMode(FilterMode mode) override;
    void bind(uint32_t) override;
    void unbind(uint32_t) override;
};

class OpenGLFramebuffer : public Framebuffer
{
public:
    OpenGLFramebuffer
    (
        uint32_t width, 
        uint32_t height, 
        TextureBuffer::InternalFormat internalFormat
    );
    ~OpenGLFramebuffer();
    void bind() override;
    void unbind() override;
    void bindColorBuffer(uint32_t) override;
    void bindDepthBuffer(uint32_t) override;
    void colorBufferData(unsigned char*, bool yFlip=false) override;
    void clearColorBuffer(float r=0,float g=0,float b=0,float a=0) override;
    void updateColorBufferMipmap() override;
};

class OpenGLVertexBuffer : public VertexBuffer
{
protected:
    void setLayout
    (
        const VertexBufferLayout&
    ) override;
public:
    OpenGLVertexBuffer(float*, uint32_t);
    ~OpenGLVertexBuffer();
    void bind() override;
    void unbind() override;
    void updateVertices(float*, uint32_t) override;
};

class OpenGLIndexBuffer : public IndexBuffer
{
public:
    OpenGLIndexBuffer(uint32_t*, uint32_t);
    ~OpenGLIndexBuffer();
    void bind() override;
    void unbind() override;
};

class OpenGLVertexArray : public VertexArray
{
public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();
    void bind() override;
    void unbind() override;
    void bindVertexBuffer(VertexBuffer*) override;
    void unbindVertexBuffer(VertexBuffer*) override;
    void bindIndexBuffer(IndexBuffer*) override;
    void unbindIndexBuffer() override;

};

}

#endif