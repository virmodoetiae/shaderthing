#ifndef V_BUFFERS_H
#define V_BUFFERS_H

#include <vector>
#include <unordered_map>
#include <iostream>
#include "vgraphics/vshader.h"

namespace vir
{

//----------------------------------------------------------------------------//

class TextureBuffer 
{
public:
    // Format of a single element stored in the texture on the device (GPU)
    // side. The number of letters before the first underscore is the number of
    // channels. The letters after the second underscore mean:
    // - UNI, unsigned normalized integer, [0-255] but treated as [0.0-1.0]
    // - UI, unsigned integer
    // - SF, signed float
    // The last number is the bit depth of each component.
    // A very limited number of formats is implemented for now
    enum class InternalFormat
    {
        Undefined,
        R_UNI_8,
        R_UI_8,
        RG_UNI_8,
        RG_UI_8,
        RGB_UNI_8,
        RGB_UI_8,
        RGBA_UNI_8,
        RGBA_UI_8,
        RGBA_SF_32
    };
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
    static const std::unordered_map<InternalFormat, std::string> 
        internalFormatToName;
    static const std::unordered_map<WrapMode, std::string> wrapModeToName;
    static const std::unordered_map<FilterMode, std::string> filterModeToName;
protected:
    uint32_t id_;
    uint32_t nDimensions_;
    uint32_t nChannels_;
    InternalFormat internalFormat_;
    WrapMode wrapModes_[3];
    FilterMode magFilterMode_;
    FilterMode minFilterMode_;
    TextureBuffer(InternalFormat internalFormat=InternalFormat::Undefined):
    id_(0),
    nDimensions_(0),
    nChannels_(nChannels(internalFormat)),
    internalFormat_(internalFormat)
    {
        for (int i=0; i<3; i++) 
            wrapModes_[i] = WrapMode::ClampToBorder;
        magFilterMode_ = FilterMode::Nearest;
        minFilterMode_ = FilterMode::LinearMipmapLinear;
    }
public:
    virtual ~TextureBuffer(){}
    uint32_t id() const {return id_;}
    uint32_t nChannels() const {return nChannels_;}
    InternalFormat internalFormat() const {return internalFormat_;}
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
    static uint32_t nChannels(InternalFormat internalFormat)
    {
        switch (internalFormat)
        {
            case InternalFormat::Undefined :
                return 0;
            case InternalFormat::R_UNI_8 :
            case InternalFormat::R_UI_8 :
                return 1;
            case InternalFormat::RG_UNI_8 :
            case InternalFormat::RG_UI_8 :
                return 2;
            case InternalFormat::RGB_UNI_8 :
            case InternalFormat::RGB_UI_8 :
                return 3;
            case InternalFormat::RGBA_UNI_8 :
            case InternalFormat::RGBA_UI_8 :
                return 4;
            case InternalFormat::RGBA_SF_32 :
                return 4;
        }
    }
    static InternalFormat defaultInternalFormat(uint32_t nChannels)
    {
        switch (nChannels)
        {
            case 1 :
                return InternalFormat::R_UNI_8;
            case 2 :
                return InternalFormat::RG_UNI_8;
            case 3 : 
                return InternalFormat::RGB_UNI_8;
            case 4 :
                return InternalFormat::RGBA_UNI_8;
        }
        return InternalFormat::Undefined;
    }
};

//----------------------------------------------------------------------------//

class TextureBuffer2D : public TextureBuffer
{
protected:
    uint32_t width_;
    uint32_t height_;
    
    TextureBuffer2D(){nDimensions_=2;}
    TextureBuffer2D
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
    ):TextureBuffer(internalFormat),width_(width),height_(height)
    {
        nDimensions_=2;
    }
public:
    ~TextureBuffer2D(){}
    static TextureBuffer2D* create
    (
        std::string, 
        InternalFormat internalFormat = InternalFormat::Undefined
    );
    static TextureBuffer2D* create
    (
        const unsigned char* data, 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
    );
    uint32_t width() const {return width_;}
    uint32_t height() const {return height_;}
    
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
        InternalFormat internalFormat
    ):TextureBuffer2D(nullptr, width, height, internalFormat){}
public:
    ~CubeMapBuffer(){}
    static CubeMapBuffer* create
    (
        std::string filepaths[6], 
        InternalFormat internalFormat = InternalFormat::Undefined
    );
    static CubeMapBuffer* create
    (
        const unsigned char* faceData[6], 
        uint32_t width,
        uint32_t height,
        InternalFormat internalFormat
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
    static Framebuffer* create
    (
        uint32_t width, 
        uint32_t height, 
        TextureBuffer::InternalFormat format = 
            TextureBuffer::InternalFormat::RGBA_UNI_8
    );
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
    uint32_t colorBufferDataSize() const {return width_*height_*4;}
    TextureBuffer::InternalFormat colorBufferInternalFormat() const
    {
        return colorBuffer_->internalFormat();
    }
    TextureBuffer::WrapMode colorBufferWrapMode(uint32_t index) const
    {
        if (colorBuffer_ == nullptr)
            return TextureBuffer::WrapMode::ClampToBorder;
        return colorBuffer_->wrapMode(index);
    }
    TextureBuffer::FilterMode colorBufferMagFilterMode() const
    {
        if (colorBuffer_ == nullptr)
            return TextureBuffer::FilterMode::Nearest;
        return colorBuffer_->magFilterMode();
    }
    TextureBuffer::FilterMode colorBufferMinFilterMode() const
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