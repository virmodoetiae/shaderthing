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

GLint OpenGLInternalFormat(TextureBuffer::InternalFormat internalFormat);

GLint OpenGLFormat(TextureBuffer::InternalFormat internalFormat);

GLint OpenGLType(TextureBuffer::InternalFormat internalFormat);

class OpenGLTextureBuffer2D : public TextureBuffer2D
{
public:
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
    void updateMipmap(bool onlyIfRequiredByFilterMode=true) override;
    void bind(uint32_t unit) override;
    void bindImage(uint32_t unit, uint32_t level, ImageBindMode mode) override;
    void unbind() override;
    void unbindImage() override;
    void readData(unsigned char*& data, bool allocate=false) override;
    void readData(unsigned int*& data, bool allocate=false) override;
    void readData(float*& data, bool allocate=false) override;
};

class OpenGLAnimatedTextureBuffer2D : public AnimatedTextureBuffer2D
{
private:
    static uint32_t nextFreeId_; // Global Id counter since id of animation
                                 // container not tied to any TextureBuffer2D 
                                 // instance
public:
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
    void updateMipmap(bool onlyIfRequiredByFilterMode=true) override;
    void bind(uint32_t) override;
    void bindImage(uint32_t unit, uint32_t level, ImageBindMode mode) override;
    void unbind() override;
    void unbindImage() override;
    void readData(unsigned char*& data, bool allocate=false) override;
    void readData(unsigned int*& data, bool allocate=false) override;
    void readData(float*& data, bool allocate=false) override;
};

class OpenGLCubeMapBuffer : public CubeMapBuffer
{
public:
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
    void updateMipmap(bool onlyIfRequiredByFilterMode=true) override;
    void bind(uint32_t) override;
    void bindImage(uint32_t unit, uint32_t level, ImageBindMode mode) override;
    void unbind() override;
    void unbindImage() override;
    void readData(unsigned char*& data, bool allocate=false) override;
    void readData(unsigned int*& data, bool allocate=false) override;
    void readData(float*& data, bool allocate=false) override;
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
    void bindColorBufferToImage
    (
        uint32_t unit, 
        uint32_t level, 
        TextureBuffer::ImageBindMode mode
    ) override;
    void bindDepthBuffer(uint32_t) override;
    void unbindColorBuffer() override;
    void unbindColorBufferFromImage() override;
    void readColorBufferData(unsigned char*& data, bool yFlip=false, bool allocate=false) override;
    void readColorBufferData(unsigned int*& data, bool yFlip=false, bool allocate=false) override;
    void readColorBufferData(float*& data, bool yFlip=false, bool allocate=false) override;
    void clearColorBuffer(float r=0,float g=0,float b=0,float a=0) override;
    void updateColorBufferMipmap(bool onlyIfRequiredByFilterMode=true) override;
private:
    template<typename DataType>
    void readColorBufferData(DataType*& data, bool yFlip, bool allocate, GLint glDataType);
};

class OpenGLUniformBuffer : public UniformBuffer
{
protected :
public :
    OpenGLUniformBuffer(uint32_t size);
    ~OpenGLUniformBuffer();
    void bind() override;
    void unbind() override;
    void setBindingPoint(uint32_t) override;
    void setData
    (
        void* data,
        uint32_t size = 0,
        uint32_t offset = 0
    ) override;
};

class OpenGLShaderStorageBuffer : public ShaderStorageBuffer
{
protected :
public :
    OpenGLShaderStorageBuffer(uint32_t size);
    ~OpenGLShaderStorageBuffer();
    bool canRunOnDeviceInUse() const override;
    void bind() override;
    void unbind() override;
    void setBindingPoint(uint32_t) override;
    void* mapData
    (
        uint32_t size = 0,
        uint32_t offset = 0
    ) override;
    void memoryBarrier() override;
    void fenceSync();
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