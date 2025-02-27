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

#include "vir/include/vir.h"
#include "shaderthing/include/macros.h"
#include "shaderthing/include/filedialog.h"

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
    Type                                         type_;
    bool                                         isNameManaged_ = true;
    std::string*                                 namePtr_       = nullptr;
    int                                          textureUnit_   = -1;
    int                                          imageUnit_     = -1;
    // List of uniforms using this resource as value
    std::vector<Uniform*>                        clientUniforms_ = {};

    static std::map<Resource::Type, const char*> typeToName_;
    static FileDialog                            fileDialog_;
    static const Resource*                       resourceToBeExported_;
    static Resource**                            resourceToBeReplaced_;
    
    Resource(Type type):type_(type){};
    DELETE_COPY_MOVE(Resource)

public:
    
    virtual ~Resource();
    static Resource*     create(const std::string& filepath);
    static Resource*     create(const unsigned char* rawData, unsigned int size, bool gif);
    static Resource*     create(unsigned int width, unsigned int height, InternalFormat internalFormat);
    static Resource*     create(unsigned int width, unsigned int height, unsigned int depth, InternalFormat internalFormat);
    static Resource*     create(const std::vector<Texture2DResource*>& frames);
    static Resource*     create(const Texture2DResource* faces[6]);
    static Resource*     create(Layer* layer);
    
    Type                 type() const {return type_;}
    virtual void         bind(unsigned int unit) = 0;
    virtual void         bindImage(unsigned int unit, unsigned int level, ImageBindMode bindMode) = 0;
    virtual void         unbind() = 0;
    virtual void         unbindImage() = 0;
    virtual unsigned int id() const = 0;
    virtual unsigned int width() const = 0;
    virtual unsigned int height() const = 0;
    virtual unsigned int depth() const {return 1;}
    virtual unsigned int nChannels() const = 0;
    virtual WrapMode     wrapMode(int index) const = 0;
    virtual FilterMode   magFilterMode() const = 0;
    virtual FilterMode   minFilterMode() const = 0;
    virtual InternalFormat internalFormat() const = 0;
    virtual DataType     dataType() const = 0;
    virtual std::string  internalFormatName() const = 0;
    virtual uint64_t     maxMemoryFootprint() const = 0;
    virtual bool         isInternalFormatUnsigned() const = 0;
    virtual void         setWrapMode(int index, WrapMode mode) = 0;
    virtual void         setMagFilterMode(FilterMode mode) = 0;
    virtual void         setMinFilterMode(FilterMode mode) = 0;
    virtual void         updateMipmap() = 0;

    std::string          name() const {return namePtr_ == nullptr? "" : *namePtr_;}
    void                 setName(const std::string& name);
    void                 setNamePtr(std::string* namePtr);
    unsigned int         textureUnit() const {return textureUnit_;}
    unsigned int         imageUnit() const {return imageUnit_;}
    void                 addClientUniform(Uniform* u) {clientUniforms_.push_back(u);}
    void                 removeClientUniform(Uniform* u) {clientUniforms_.erase(std::remove(clientUniforms_.begin(), clientUniforms_.end(), u), clientUniforms_.end());}
    bool                 isUsedByUniform(const Uniform* u) const {return std::find(clientUniforms_.begin(), clientUniforms_.end(), u) != clientUniforms_.end();}

    static bool isGuiOpen;
    static bool isGuiDetachedFromMenu;
    static void renderResourcesGui
    (
        std::vector<Resource*>& resources, 
        const std::vector<Layer*>& layers
    );
    static void renderResourcesMenuItemGui
    (
        std::vector<Resource*>& resources,
        const std::vector<Layer*>& layers
    );
    static void update
    (
        std::vector<Resource*>& resources,
        const UpdateArgs& args
    );
    static void saveAll
    (
        const std::vector<Resource*>& resources,
        ObjectIO& io
    );
    static void loadAll
    (
        const ObjectIO& io,
        std::vector<Resource*>& resources
    );
    static void resetAnimationsTime
    (
        const std::vector<Resource*>& resources, 
        float time=0.f
    );
    static void prepareAnimationsForExport
    (
        const std::vector<Resource*>& resources,
        float startTime, 
        bool forceResumeTime,
        bool cacheTime=false
    );
    static void resetAnimationsAfterExport
    (
        const std::vector<Resource*>& resources
    );

private:
    
    virtual void update(const UpdateArgs& args){(void)args;};
    virtual void save(ObjectIO& io) = 0;
    static Resource* load
    (
        const ObjectIO& io, 
        const std::vector<Resource*>& resources
    );
    
    static bool loadOrReplaceTexture2DOrAnimationButtonGui
    (
        Resource*& resource,
        const ImVec2 size=ImVec2(0,0),
        const bool animation=false,
        const bool disabled=false
    );
    static bool createOrResizeOrReformatTexture2DGui
    (
        Resource*& resource,
        const bool enablePopup,
        const bool resetValues,
        const ImVec2 buttonSize=ImVec2(0,0)
    );
    static bool createOrResizeOrReformatTexture3DGui
    (
        Resource*& resource,
        const bool enablePopup,
        const bool resetValues,
        const ImVec2 buttonSize=ImVec2(0,0)
    );
    static void createOrResizeOrReformatTextureMemoryEstimateGui
    (
        uint64_t textureSize,
        InternalFormat internalFormat,
        bool is2D
    );
    static bool createOrEditAnimationButtonGui
    (
        Resource*& resource,
        const std::vector<Resource*>& resources,
        const ImVec2 size=ImVec2(0,0)
    );
    static bool createOrEditCubemapButtonGui
    (
        Resource*& resource,
        const std::vector<Resource*>& resources,
        const ImVec2 size=ImVec2(0,0)
    );
    static bool exportTexture2DOrAnimationButtonGui
    (
        const Resource* resource,
        const ImVec2 size=ImVec2(0,0)
    );
    static void renderResourceActionsButtonGui
    (
        Resource*& resource,
        bool& deleteResource,
        const std::vector<Resource*>& resources,
        const ImVec2 size=ImVec2(0,0)
    );
};

#define DECLARE_OVERRIDE_VIRTUALS                                                                     \
    void         bind(unsigned int unit) override {native_->bind(unit); textureUnit_ = unit;}         \
    void         unbind() override {native_->unbind(); textureUnit_ = -1;};                           \
    void         bindImage(unsigned int unit, unsigned int level, ImageBindMode bindMode) override {native_->bindImage(unit, level, bindMode); imageUnit_ = unit;} \
    void         unbindImage() override {native_->unbindImage(); imageUnit_ = -1;};                   \
    unsigned int id() const override {return native_->id();}                                          \
    unsigned int width() const override {return native_->width();}                                    \
    unsigned int height() const override{return native_->height();}                                   \
    unsigned int nChannels() const override{return native_->nChannels();}                             \
    WrapMode     wrapMode(int index) const override {return native_->wrapMode(index);}                \
    FilterMode   magFilterMode() const override {return native_->magFilterMode();}                    \
    FilterMode   minFilterMode() const override {return native_->minFilterMode();}                    \
    InternalFormat internalFormat() const override {return native_->internalFormat();}                \
    DataType     dataType() const override {return native_->dataType();}                              \
    std::string  internalFormatName() const override {return vir::TextureBuffer::internalFormatToShortName.at(native_->internalFormat());} \
    bool         isInternalFormatUnsigned() const override {return native_->isInternalFormatUnsigned();} \
    void         setWrapMode(int index, WrapMode mode) override {native_->setWrapMode(index, mode);}  \
    void         setMagFilterMode(FilterMode mode) override {native_->setMagFilterMode(mode);}        \
    void         setMinFilterMode(FilterMode mode) override {native_->setMinFilterMode(mode);}        \
    void         updateMipmap() override {native_->updateMipmap(true);}

class Texture2DResource : public Resource
{
    friend                  Resource;
    friend AnimatedTexture2DResource;
    friend           CubemapResource;
    
    vir::TextureBuffer2D* native_      = nullptr;
    const unsigned char*  rawData_     = nullptr;
    unsigned int          rawDataSize_ = 0;
    std::string           originalFileExtension_;
    
    Texture2DResource():Resource(Type::Texture2D){}
    DELETE_COPY_MOVE(Texture2DResource)

    virtual void save(ObjectIO& io) override;
    static Texture2DResource* load(const ObjectIO& io);
public:
    bool autoUpdateMipmap = false;
   
    ~Texture2DResource();
    bool set(const std::string& filepath);
    bool set(const unsigned char* rawData, unsigned int size);
    bool set(unsigned int width, unsigned int height, InternalFormat internalFormat);
    void update(const UpdateArgs& args) override;
    void readData(unsigned char*& data, bool allocate=false) const {native_->readData(data, allocate);}
    void readData(unsigned int*& data, bool allocate=false) const {native_->readData(data, allocate);}
    void readData(float*& data, bool allocate=false) const {native_->readData(data, allocate);}
    bool hasRawData() const {return rawData_ != nullptr;}
    uint64_t maxMemoryFootprint() const override {return native_->maxMemoryFootprint();}
    DECLARE_OVERRIDE_VIRTUALS
};

class AnimatedTexture2DResource : public Resource
{
    friend Resource;
    
    vir::AnimatedTextureBuffer2D*   native_                       = nullptr;
    const unsigned char*            rawData_                      = nullptr;
    unsigned int                    rawDataSize_                  = 0;
    std::string                     originalFileExtension_;
    std::vector<Texture2DResource*> unmanagedFrames_;
    bool                            isAnimationPaused_            = false;
    bool                            isAnimationBoundToGlobalTime_ = false;
    float                           cachedTime_ = 0.f;
    
    AnimatedTexture2DResource():Resource(Type::AnimatedTexture2D){}
    DELETE_COPY_MOVE(AnimatedTexture2DResource)

    virtual void save(ObjectIO& io);
    static AnimatedTexture2DResource* load
    (
        const ObjectIO& io,
        const std::vector<Resource*>& resources
    );
public:
    bool autoUpdateMipmap = false;

    ~AnimatedTexture2DResource();
    bool set(const std::string& filepath);
    bool set(const unsigned char* rawData, unsigned int size);
    bool set(const std::vector<Texture2DResource*>& animationFrames);
    void update(const UpdateArgs& args) override;
    unsigned int frameId() const {return native_->frameId();}
    uint64_t maxMemoryFootprint() const override {return native_->maxMemoryFootprint();}
    DECLARE_OVERRIDE_VIRTUALS
};

class CubemapResource : public Resource
{
    friend Resource;
    
    vir::CubeMapBuffer*      native_ = nullptr;
    const Texture2DResource* unmanagedFaces_[6];
    
    CubemapResource():Resource(Type::Cubemap){}
    DELETE_COPY_MOVE(CubemapResource)

    virtual void save(ObjectIO& io);
    static CubemapResource* load
    (
        const ObjectIO& io,
        const std::vector<Resource*>& resources
    );
public:
    ~CubemapResource();
    bool set(const Texture2DResource* faces[6]);
    uint64_t maxMemoryFootprint() const override {return native_->maxMemoryFootprint();}
    DECLARE_OVERRIDE_VIRTUALS
};

class Texture3DResource : public Resource
{
    friend                  Resource;
    
    vir::TextureBuffer3D* native_      = nullptr;
    
    Texture3DResource():Resource(Type::Texture3D){}
    DELETE_COPY_MOVE(Texture3DResource)

    virtual void save(ObjectIO& io) override;
    static Texture3DResource* load(const ObjectIO& io);
public:
    bool autoUpdateMipmap = false;
   
    ~Texture3DResource();
    bool set(unsigned int width, unsigned int height, unsigned int depth, InternalFormat internalFormat);
    void update(const UpdateArgs& args) override;
    void readData(unsigned char*& data, bool allocate=false) const {native_->readData(data, allocate);}
    void readData(unsigned int*& data, bool allocate=false) const {native_->readData(data, allocate);}
    void readData(float*& data, bool allocate=false) const {native_->readData(data, allocate);}
    DECLARE_OVERRIDE_VIRTUALS
    unsigned int depth() const override {return native_->depth();}
    uint64_t maxMemoryFootprint() const override {return native_->maxMemoryFootprint();}
};

class Layer;
class LayerResource : public Resource
{
    friend Resource;
    Layer*             layer_  = nullptr;
    vir::Framebuffer** native_ = nullptr;
    LayerResource():Resource(Type::Framebuffer){isNameManaged_=false;}
    
    DELETE_COPY_MOVE(LayerResource)

    virtual void save(ObjectIO& io){(void)io;}
public:
    ~LayerResource();
    bool         set(Layer* layer);
    void         bind(unsigned int unit) override {(*native_)->bindColorBuffer(unit); textureUnit_=unit;}
    void         unbind() override {(*native_)->unbindColorBuffer(); textureUnit_=-1;};
    void         bindImage(unsigned int unit, unsigned int level, ImageBindMode mode) override {(*native_)->bindColorBufferToImage(unit, level, mode); imageUnit_=unit;}
    void         unbindImage() override {(*native_)->unbindColorBufferFromImage(); imageUnit_=-1;};
    unsigned int id() const override {return (*native_)->colorBufferId();}
    unsigned int width() const override {return (*native_)->width();}
    unsigned int height() const override {return (*native_)->height();}
    unsigned int nChannels() const override {return (*native_)->colorBufferNChannels();}
    WrapMode     wrapMode(int index) const override {return (*native_)->colorBufferWrapMode(index);}
    FilterMode   magFilterMode() const override {return (*native_)->colorBufferMagFilterMode();}
    FilterMode   minFilterMode() const override {return (*native_)->colorBufferMinFilterMode();}
    InternalFormat internalFormat() const override {return (*native_)->colorBufferInternalFormat();}
    DataType     dataType() const override {return (*native_)->colorBufferDataType();}
    std::string  internalFormatName() const override {return vir::TextureBuffer::internalFormatToShortName.at((*native_)->colorBufferInternalFormat());}
    uint64_t     maxMemoryFootprint() const override {return 0;}
    bool         isInternalFormatUnsigned() const override {return false;}
    void         setWrapMode(int index, WrapMode mode) override {(*native_)->setColorBufferWrapMode(index, mode);}
    void         setMagFilterMode(FilterMode mode) override {(*native_)->setColorBufferMagFilterMode(mode);}
    void         setMinFilterMode(FilterMode mode) override {(*native_)->setColorBufferMinFilterMode(mode);}
    void         updateMipmap() override {(*native_)->updateColorBufferMipmap(true);}

    static bool insertInResources
    (
        Layer* layer,
        std::vector<Resource*>& resources
    );
    static bool removeFromResources
    (
        const Layer* layer,
        std::vector<Resource*>& resources
    );
};

}