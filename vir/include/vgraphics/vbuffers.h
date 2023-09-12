#ifndef V_BUFFERS_H
#define V_BUFFERS_H

#include <vector>
#include <unordered_map>
#include "vgraphics/vshader.h"

namespace vir
{

//----------------------------------------------------------------------------//

class TextureBuffer 
{
public:
    enum class WrapMode
    {
        ClampToBorder,
        ClampToEdge,
        Repeat,
        MirroredRepeat
    };
    enum class FilterMode
    {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };
    static const std::unordered_map<WrapMode, std::string> wrapModeToName;
    static const std::unordered_map<FilterMode, std::string> filterModeToName;
protected:
    uint32_t id_;
    uint32_t nDimensions_;
    WrapMode wrapModes_[3];
    FilterMode magFilterMode_;
    FilterMode minFilterMode_;
    TextureBuffer():
    id_(0)
    {
        for (int i=0; i<3; i++) 
            wrapModes_[i] = WrapMode::ClampToBorder;
        magFilterMode_ = FilterMode::Nearest;
        minFilterMode_ = FilterMode::LinearMipmapLinear;
    }
public:
    virtual ~TextureBuffer(){}
    uint32_t id() const {return id_;}
    WrapMode wrapMode(uint32_t index) const {return wrapModes_[index];}
    FilterMode magFilterMode() const {return magFilterMode_;}
    FilterMode minFilterMode() const {return minFilterMode_;}
    virtual void bind(uint32_t) = 0;
    virtual void unbind(uint32_t) = 0;
    virtual void setWrapMode
    (
        uint32_t index,
        WrapMode value
    ) = 0;
    virtual void setMagFilterMode(FilterMode mode) = 0;
    virtual void setMinFilterMode(FilterMode mode) = 0;
    bool operator==(const TextureBuffer& rhs) const
    {
        return id_ == rhs.id();
    }
};

//----------------------------------------------------------------------------//

class TextureBuffer2D : public TextureBuffer
{
protected:
    uint32_t width_;
    uint32_t height_;
    uint32_t nChannels_;
    TextureBuffer2D(){nDimensions_=2;}
    TextureBuffer2D
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        uint32_t nChannels
    ):width_(width),height_(height),nChannels_(nChannels){}
public:
    ~TextureBuffer2D(){}
    static TextureBuffer2D* create(std::string, uint32_t requestedChannels=0);
    static TextureBuffer2D* create
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        uint32_t nChannels
    );
    uint32_t width(){return width_;}
    uint32_t height(){return height_;}
    uint32_t nChannels(){return nChannels_;}
};

//----------------------------------------------------------------------------//

class CubeMapBuffer : public TextureBuffer2D
{
protected:
    CubeMapBuffer(){}
    CubeMapBuffer
    (
        const unsigned char* faceData[6], 
        uint32_t width,
        uint32_t height,
        uint32_t nChannels
    ):TextureBuffer2D(nullptr, width, height, nChannels){}
public:
    ~CubeMapBuffer(){}
    static CubeMapBuffer* create
    (
        std::string filepaths[6], 
        uint32_t requestedChannels=0
    );
    static CubeMapBuffer* create
    (
        const unsigned char* faceData[6], 
        uint32_t width,
        uint32_t height,
        uint32_t nChannels
    );
};

//----------------------------------------------------------------------------//

class Framebuffer
{
protected:
    //
    static Framebuffer* activeOne_;
    uint32_t id_;
    uint32_t colorBufferId_;
    uint32_t depthBufferId_;
    uint32_t width_;
    uint32_t height_;
    TextureBuffer2D* colorBuffer_;
    Framebuffer():id_(0){};
public:
    virtual ~Framebuffer(){}
    static Framebuffer* create(uint32_t, uint32_t);
    static Framebuffer*& activeOne(){return activeOne_;}
    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void bindColorBuffer(uint32_t) = 0;
    virtual void bindDepthBuffer(uint32_t) = 0;
    virtual void colorBufferData(unsigned char*, bool yFlip=false) = 0;
    uint32_t id() const {return id_;}
    uint32_t colorBufferId() const {return colorBufferId_;}
    uint32_t width() const {return width_;}
    uint32_t height() const {return height_;}
    uint32_t colorBufferDataSize(){return width_*height_*4;}
    TextureBuffer::WrapMode colorBufferWrapMode(uint32_t index)
    {
        if (colorBuffer_ == nullptr)
            return TextureBuffer::WrapMode::ClampToBorder;
        return colorBuffer_->wrapMode(index);
    }
    TextureBuffer::FilterMode colorBufferMagFilterMode()
    {
        if (colorBuffer_ == nullptr)
            return TextureBuffer::FilterMode::Nearest;
        return colorBuffer_->magFilterMode();
    }
    TextureBuffer::FilterMode colorBufferMinFilterMode()
    {
        if (colorBuffer_ == nullptr)
            return TextureBuffer::FilterMode::Nearest;
        return colorBuffer_->minFilterMode();
    }
    void setColorBufferWrapMode(uint32_t index, TextureBuffer::WrapMode mode)
    {
        if (colorBuffer_ == nullptr)
            return;
        colorBuffer_->setWrapMode(index, mode);
    }
    void setColorBufferMagFilterMode(TextureBuffer::FilterMode mode)
    {
        if (colorBuffer_ == nullptr)
            return;
        colorBuffer_->setMagFilterMode(mode);
    }
    void setColorBufferMinFilterMode(TextureBuffer::FilterMode mode)
    {
        if (colorBuffer_ == nullptr)
            return;
        colorBuffer_->setMinFilterMode(mode);
    }
    bool operator==(const TextureBuffer& rhs) const
    {
        return id_ == rhs.id();
    }
};

//----------------------------------------------------------------------------//

class GraphicsBuffer
{
protected :

    // Element count of the buffer (vertices, indices, etc.)
    uint32_t count_;

    //
    uint32_t id_;

    // Protected constructor
    GraphicsBuffer():count_(0),id_(0){}
    
    // Deleted all other constructors
    GraphicsBuffer(const GraphicsBuffer&) = delete;
    GraphicsBuffer& operator=(const GraphicsBuffer& other) = delete;
    GraphicsBuffer(GraphicsBuffer&&) = delete;
    GraphicsBuffer& operator=(GraphicsBuffer&& other) = delete;

public :

    virtual ~GraphicsBuffer(){}

    virtual void bind() = 0;
    virtual void unbind() = 0;

    // Accessors
    uint32_t count() const {return count_;}
    uint32_t id() const {return id_;}

    bool operator==(const GraphicsBuffer& rhs) const
    {
        return id_ == rhs.id();
    }

};

//----------------------------------------------------------------------------//

class VertexBufferLayout
{
public:
    struct Element
    {
        std::string name;
        Shader::Variable variable;
        bool normalized;
        uint32_t offset = 0;
        uint32_t location = 0;
        Element(std::string& n, Shader::Variable& v, bool norm=false)
        {
            name = n;
            variable = v;
            normalized = norm;
        }
        Element(std::string&& n, Shader::Variable&& v, bool norm=false)
        {
            name = n;
            variable = v;
            normalized = norm;
        }
    };
protected:
    uint32_t stride_;
    std::vector<Element> elements_;
public:
    VertexBufferLayout(const std::initializer_list<Element>& elements);
    const std::vector<Element>& elements() const {return elements_;}
    uint32_t stride() const {return stride_;}
};

//----------------------------------------------------------------------------//

class VertexArray;

typedef std::vector<VertexArray*> VertexArrayPtrVector;

class VertexBuffer : public GraphicsBuffer
{
protected:

    // Parent vertex arrays that reference this buffer (only for OpenGL, really)
    VertexArrayPtrVector vertexArrays_;

    // Layout of this vertex buffer (always tied to the shader)
    VertexBufferLayout* layout_ = nullptr;

    virtual void setLayout(const VertexBufferLayout&) = 0;

public:

    // Static virtual-like constructor
    static VertexBuffer* create(float*, uint32_t);
    
    virtual ~VertexBuffer();
    
    void setLayout
    (
        const std::initializer_list<VertexBufferLayout::Element>& elements
    );
    void setLayout();

    virtual void updateVertices(float*, uint32_t) = 0;

    VertexArrayPtrVector& vertexArrays(){return vertexArrays_;}
};

//----------------------------------------------------------------------------//

class IndexBuffer : public GraphicsBuffer
{
protected:

    // Parent vertex arrays that reference this buffer (only for OpenGL, really)
    VertexArrayPtrVector vertexArrays_;

public:

    // Static virtual-like constructor
    static IndexBuffer* create(uint32_t*, uint32_t);
    
    virtual ~IndexBuffer();

    VertexArrayPtrVector& vertexArrays(){return vertexArrays_;}
};

//----------------------------------------------------------------------------//

typedef std::vector<VertexBuffer*> VertexBufferPtrVector;

// A vertex array is not really a buffer but the inheritance is kinda useful 
// here as most members are the same
class VertexArray : public GraphicsBuffer
{
protected:

    //
    static VertexArray* activeOne_;
    
    // List of vertex buffers bound to this VertexArray
    VertexBufferPtrVector vertexBuffers_;

    // Index buffer bound to this VertexArray
    IndexBuffer* indexBuffer_ = nullptr;

public:

    static VertexArray*& activeOne(){return activeOne_;}

    static VertexArray* create();
    
    virtual ~VertexArray(){}
    
    virtual void bindVertexBuffer(VertexBuffer*) = 0;
    virtual void unbindVertexBuffer(VertexBuffer*) = 0;
    virtual void bindIndexBuffer(IndexBuffer*) = 0;
    virtual void unbindIndexBuffer() = 0;

    VertexBufferPtrVector& vertexBuffers(){return vertexBuffers_;}
    IndexBuffer*& indexBuffer(){return indexBuffer_;}

};

}

#endif