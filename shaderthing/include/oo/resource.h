/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <array>
#include "vir/include/vir.h"
#include "shaderthing/include/macros.h"
#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

typedef vir::TextureBuffer::WrapMode       WrapMode;
typedef vir::TextureBuffer::FilterMode     FilterMode;
typedef vir::TextureBuffer::InternalFormat InternalFormat;
typedef vir::TextureBuffer::ImageBindMode  ImageBindMode;
typedef vir::TextureBuffer::DataType       DataType;

class Layer;
class Uniform;
class ObjectIO;

class Texture2DResource;
class AnimatedTexture2DResource;
class CubemapResource;
class LayerResource;

class Resource
{
public:

    enum class Type
    {
        Texture2D,
        Texture3D,
        AnimatedTexture2D,
        Cubemap,
        Framebuffer
    };
    struct UpdateArgs
    {
        const float time;
        const float timeStep;
    };

protected:

    const Type            type_;
    std::string*          namePtr_       = nullptr;
    // Ture if namePtr_ is owned by this resource (false e.g. if this resource
    // references a Layer, and thus namePtr_ points to the layer name and is
    // thus not owned)
    bool                  isNameManaged_ = true;
    int                   textureUnit_   = -1;
    int                   imageUnit_     = -1;
    // List of uniforms using this resource as value
    std::vector<Uniform*> clientUniforms_ = {};
    
    Resource(Type type) : type_(type) {};
    NO_COPY(Resource)

public:
    
    virtual ~Resource();
    /*
    static Resource*     create(const std::string& filepath);
    static Resource*     create(const unsigned char* rawData, unsigned int size, bool gif);
    static Resource*     create(unsigned int width, unsigned int height, InternalFormat internalFormat);
    static Resource*     create(unsigned int width, unsigned int height, unsigned int depth, InternalFormat internalFormat);
    static Resource*     create(const std::vector<Texture2DResource*>& frames);
    static Resource*     create(const Texture2DResource* faces[6]);
    static Resource*     create(Layer* layer);
    */

    Type                   type() const {return type_;}
    unsigned int           textureUnit() const {return textureUnit_;}
    unsigned int           imageUnit() const {return imageUnit_;}
    
    std::string            name() const;
    void                   setName(const std::string& name);
    void                   setName(std::string* namePtr);
    void                   addClientUniform(Uniform* u);
    void                   removeClientUniform(Uniform* u);
    bool                   isUsedByUniform(const Uniform *u) const;

    virtual void           save(ObjectIO& io) = 0;
    virtual void           update(const UpdateArgs& args) = 0;
    
    virtual void           bind(unsigned int unit) = 0;
    virtual void           bindImage
    (
        unsigned int unit, 
        unsigned int level, 
        ImageBindMode bindMode
    ) = 0;
    virtual void           unbind() = 0;
    virtual void           unbindImage() = 0;
    virtual unsigned int   id() const = 0;
    virtual unsigned int   width() const = 0;
    virtual unsigned int   height() const = 0;
    virtual unsigned int   depth() const {return 1;}
    virtual unsigned int   nChannels() const = 0;
    virtual WrapMode       wrapMode(int index) const = 0;
    virtual FilterMode     magFilterMode() const = 0;
    virtual FilterMode     minFilterMode() const = 0;
    virtual InternalFormat internalFormat() const = 0;
    virtual DataType       dataType() const = 0;
    virtual std::string    internalFormatName() const = 0;
    virtual uint64_t       maxMemoryFootprint() const = 0;
    virtual bool           isInternalFormatUnsigned() const = 0;
    virtual void           setWrapMode(int index, WrapMode mode) = 0;
    virtual void           setMagFilterMode(FilterMode mode) = 0;
    virtual void           setMinFilterMode(FilterMode mode) = 0;
    virtual void           updateMipmap() = 0;

    static const std::map<Resource::Type, const char*> typeToName;
};

//----------------------------------------------------------------------------//

template <typename NativeType>
class CRTPResource : public Resource
{
protected:

    UPtr<NativeType> native_;

    CRTPResource(Type type) : Resource(type) {}
    NO_COPY(CRTPResource)

public:

    virtual ~CRTPResource();

    const NativeType* native() const {return native_.get();}

    void bind(unsigned int unit) override 
    {
        native_->bind(unit); 
        textureUnit_ = unit;
    }
    void unbind() override
    {
        native_->unbind(); 
        textureUnit_ = -1;
    }
    void bindImage
    (
        unsigned int unit, 
        unsigned int level, 
        ImageBindMode bindMode
    ) override 
    {
        native_->bindImage(unit, level, bindMode); 
        imageUnit_ = unit;
    }
    void unbindImage() override 
    {
        native_->unbindImage(); 
        imageUnit_ = -1;
    };
    unsigned int id() const override 
    {
        return native_->id();
    }
    unsigned int width() const override 
    {
        return native_->width();
    }
    unsigned int height() const override 
    {
        return native_->height();
    }
    unsigned int nChannels() const override 
    {
        return native_->nChannels();
    }
    WrapMode wrapMode(int index) const override 
    {
        return native_->wrapMode(index);
    }
    FilterMode magFilterMode() const override 
    {
        return native_->magFilterMode();
    }
    FilterMode minFilterMode() const override 
    {
        return native_->minFilterMode();
    }
    InternalFormat internalFormat() const override 
    {
        return native_->internalFormat();
    }
    DataType dataType() const override 
    {
        return native_->dataType();
    }
    std::string 
    internalFormatName() const override 
    {
        return vir::TextureBuffer::internalFormatToShortName.at
        (
            native_->internalFormat()
        );
    }
    bool isInternalFormatUnsigned() const override 
    {
        return native_->isInternalFormatUnsigned();
    }
    void setWrapMode(int index, WrapMode mode) override 
    {
        native_->setWrapMode(index, mode);
    }
    void setMagFilterMode(FilterMode mode) override 
    {
        native_->setMagFilterMode(mode);
    }
    void setMinFilterMode(FilterMode mode) override 
    {
        native_->setMinFilterMode(mode);
    }
    void updateMipmap() override 
    {
        native_->updateMipmap(true);
    }
};

//----------------------------------------------------------------------------//

class Texture2DResource : public CRTPResource<vir::TextureBuffer2D>
{
    const unsigned char*  rawData_     = nullptr;
    unsigned int          rawDataSize_ = 0;
    std::string           originalFileExtension_;
    
    Texture2DResource() : CRTPResource<vir::TextureBuffer2D>(Type::Texture2D) {}
    NO_COPY(Texture2DResource)

public:

    bool autoUpdateMipmap = false;
    
    static UPtr<Texture2DResource> create(const std::string& filepath);
    static UPtr<Texture2DResource> create
    (
        const unsigned char* rawData, 
        unsigned int size
    );
    static UPtr<Texture2DResource> create
    (
        unsigned int width, 
        unsigned int height, 
        InternalFormat internalFormat
    );

    ~Texture2DResource();

    virtual void save(ObjectIO& io) override;
    static UPtr<Texture2DResource> load(const ObjectIO& io);
    
    const unsigned char* rawData() const {return rawData_;}
    unsigned int rawDataSize() const {return rawDataSize_;}
    bool set(const std::string& filepath);
    bool set(const unsigned char* rawData, unsigned int size);
    bool set(unsigned int width, unsigned int height, InternalFormat format);
    void update(const UpdateArgs& args) override;
    void readData(unsigned char*& data, bool allocate=false) const;
    void readData(unsigned int*& data, bool allocate=false) const;
    void readData(float*& data, bool allocate=false) const;
    bool hasRawData() const 
    {
        return rawData_ != nullptr;
    }
    uint64_t maxMemoryFootprint() const override 
    {
        return native_->maxMemoryFootprint();
    }
};

//----------------------------------------------------------------------------//

class AnimatedTexture2DResource : 
    public CRTPResource<vir::AnimatedTextureBuffer2D>
{
    const unsigned char*                 rawData_                     = nullptr;
    unsigned int                         rawDataSize_                 = 0;
    std::string                          originalFileExtension_       = ".gif";
    std::vector<WPtr<Texture2DResource>> unmanagedFrames_;
    
    float                                cachedTime_ = 0.f;
    
    AnimatedTexture2DResource() : CRTPResource(Type::AnimatedTexture2D) {}
    NO_COPY(AnimatedTexture2DResource)

public:

    bool autoUpdateMipmap             = false;
    bool isAnimationPaused            = false;
    bool isAnimationBoundToGlobalTime = false;

    static UPtr<AnimatedTexture2DResource> create(const std::string& filepath);
    static UPtr<AnimatedTexture2DResource> create
    (
        const unsigned char* rawData, 
        unsigned int size
    );
    static UPtr<AnimatedTexture2DResource> create
    (
        const std::vector<WPtr<Texture2DResource>>& frames
    );

    ~AnimatedTexture2DResource();

    virtual void save(ObjectIO& io) override;
    static UPtr<AnimatedTexture2DResource> load
    (
        const ObjectIO& io,
        const std::vector<UPtr<Resource>>& resources
    );

    bool set(const std::string& filepath);
    bool set(const unsigned char* rawData, unsigned int size);
    bool set(const std::vector<WPtr<Texture2DResource>>& animationFrames);
    void update(const UpdateArgs& args) override;
    unsigned int frameId() const 
    {
        return native_->frameId();
    }
    uint64_t maxMemoryFootprint() const override 
    {
        return native_->maxMemoryFootprint();
    }
};

//----------------------------------------------------------------------------//

class CubemapResource : public CRTPResource<vir::CubeMapBuffer>
{
    std::array<WPtr<Texture2DResource>, 6> unmanagedFaces_;
    
    CubemapResource() : CRTPResource(Type::Cubemap) {}
    NO_COPY(CubemapResource)
    
public:

    ~CubemapResource() {}

    static UPtr<CubemapResource> create
    (
        const std::array<WPtr<Texture2DResource>, 6>& faces
    );

    virtual void save(ObjectIO& io) override;
    static UPtr<CubemapResource> load
    (
        const ObjectIO& io,
        const std::vector<UPtr<Resource>>& resources
    );
    
    bool set(const std::array<WPtr<Texture2DResource>, 6>& faces);
    void update(const UpdateArgs& args) override {}
    
    uint64_t maxMemoryFootprint() const override 
    {
        return native_->maxMemoryFootprint();
    }
};

}