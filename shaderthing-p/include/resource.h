#pragma once

#include "vir/include/vir.h"
#include "shaderthing-p/include/macros.h"
#include "shaderthing-p/include/filedialog.h"

namespace ShaderThing
{

typedef vir::TextureBuffer::WrapMode   WrapMode;
typedef vir::TextureBuffer::FilterMode FilterMode;

class Layer;
class ObjectIO;

class Texture2DResource;
class AnimatedTexture2DResource;
class CubemapResource;
class FramebufferResource;

class Resource
{
public:
    enum class Type
    {
        Texture2D,
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
    Type                          type_;
    bool                          isNameManaged_ = true;
    std::string*                  namePtr_       = nullptr;
    int                           unit_          = -1;

    static std::map<Resource::Type, const char*> typeToName_;
    static FileDialog                            fileDialog_;
    static const Resource*                       resourceToBeExported_;
    static Resource**                            resourceToBeReplaced_;
    
    Resource(Type type):type_(type){};
    DELETE_COPY_MOVE(Resource)

public:
    
    virtual ~Resource();
    static Resource*           create(const std::string& filepath);
    static Resource*           create(unsigned char* rawData, unsigned int size, bool gif);
    static Resource*           create(const std::vector<Texture2DResource*>& frames);
    static Resource*           create(const Texture2DResource* faces[6]);
    static Resource*           create(vir::Framebuffer** framebuffer);
    
    Type                       type() const {return type_;}
    virtual void               bind(unsigned int unit) = 0;
    virtual void               unbind() = 0;
    virtual const unsigned int id() const = 0;
    virtual const unsigned int width() const = 0;
    virtual const unsigned int height() const = 0;
    virtual const WrapMode     wrapMode(int index) const = 0;
    virtual const FilterMode   magFilterMode() const = 0;
    virtual const FilterMode   minFilterMode() const = 0;
    virtual void               setWrapMode(int index, WrapMode mode) = 0;
    virtual void               setMagFilterMode(FilterMode mode) = 0;
    virtual void               setMinFilterMode(FilterMode mode) = 0;

    std::string                name() const {return namePtr_ == nullptr? "" : *namePtr_;}
    void                       setName(const std::string& name);
    void                       setNamePtr(std::string* namePtr);

    static bool isGuiOpen;
    static bool isGuiDetachedFromMenu;
    static void renderResourcesGUI
    (
        std::vector<Resource*>& resources, 
        const std::vector<Layer*>& layers
    );
    static void renderResourcesMenuItemGUI
    (
        std::vector<Resource*>& resources,
        const std::vector<Layer*>& layers
    );
    static bool insertFramebufferInResources
    (
        std::string* name,
        vir::Framebuffer** framebuffer, 
        std::vector<Resource*>& resources
    );
    static bool removeFramebufferFromResources
    (
        vir::Framebuffer** framebuffer, 
        std::vector<Resource*>& resources
    );
    static void update
    (
        std::vector<Resource*>& resources,
        const UpdateArgs& args
    );
    static void save
    (
        const std::vector<Resource*>& resources,
        ObjectIO& io
    );
    static void loadAll
    (
        const ObjectIO& io,
        std::vector<Resource*>& resources
    );

private:
    
    virtual void update(const UpdateArgs& args){};
    virtual void save(ObjectIO& io) = 0;
    static Resource* load
    (
        const ObjectIO& io, 
        const std::vector<Resource*>& resources
    );
    
    static bool loadOrReplaceTextureOrAnimationButtonGUI
    (
        Resource*& resource,
        const ImVec2 size=ImVec2(0,0),
        const bool animation=false,
        const bool disabled=false
    );
    static bool createOrEditAnimationButtonGUI
    (
        Resource*& resource,
        const std::vector<Resource*>& resources,
        const ImVec2 size=ImVec2(0,0)
    );
    static bool createOrEditCubemapButtonGUI
    (
        Resource*& resource,
        const std::vector<Resource*>& resources,
        const ImVec2 size=ImVec2(0,0)
    );
    static bool exportTextureOrAnimationButtonGUI
    (
        const Resource* resource,
        const ImVec2 size=ImVec2(0,0)
    );
    static void renderResourceActionsButtonGUI
    (
        Resource*& resource,
        bool& deleteResource,
        const std::vector<Resource*>& resources,
        const ImVec2 size=ImVec2(0,0)
    );
};

#define DECLARE_OVERRIDE_VIRTUALS                                                                           \
    virtual void       bind(unsigned int unit) override {native_->bind(unit); unit_ = unit;}                \
    virtual void       unbind() override {native_->unbind(-1); unit_ = -1;};                                \
    const unsigned int id() const override {return native_->id();}                                          \
    const unsigned int width() const override {return native_->width();}                                    \
    const unsigned int height() const override{return native_->height();}                                   \
    const WrapMode     wrapMode(int index) const override {return native_->wrapMode(index);}                \
    const FilterMode   magFilterMode() const override {return native_->magFilterMode();}                    \
    const FilterMode   minFilterMode() const override {return native_->minFilterMode();}                    \
    virtual void       setWrapMode(int index, WrapMode mode) override {native_->setWrapMode(index, mode);}  \
    virtual void       setMagFilterMode(FilterMode mode) override {native_->setMagFilterMode(mode);}        \
    virtual void       setMinFilterMode(FilterMode mode) override{native_->setMinFilterMode(mode);}

class Texture2DResource : public Resource
{
    friend                  Resource;
    friend AnimatedTexture2DResource;
    friend           CubemapResource;
    
    vir::TextureBuffer2D* native_      = nullptr;
    unsigned char*        rawData_     = nullptr;
    unsigned int          rawDataSize_ = 0;
    std::string           originalFileExtension_;
    
    Texture2DResource():Resource(Type::Texture2D){}
    DELETE_COPY_MOVE(Texture2DResource)

    virtual void save(ObjectIO& io) override;
    static Texture2DResource* load(const ObjectIO& io);
public:
    ~Texture2DResource();
    bool set(const std::string& filepath);
    bool set(unsigned char* rawData, unsigned int size);
    DECLARE_OVERRIDE_VIRTUALS
};

class AnimatedTexture2DResource : public Resource
{
    friend Resource;
    
    vir::AnimatedTextureBuffer2D*   native_                       = nullptr;
    unsigned char*                  rawData_                      = nullptr;
    unsigned int                    rawDataSize_                  = 0;
    std::string                     originalFileExtension_;
    std::vector<Texture2DResource*> unmanagedFrames_;
    bool                            isAnimationPaused_            = false;
    bool                            isAnimationBoundToGlobalTime_ = false;
    
    AnimatedTexture2DResource():Resource(Type::AnimatedTexture2D){}
    DELETE_COPY_MOVE(AnimatedTexture2DResource)

    virtual void save(ObjectIO& io);
    static AnimatedTexture2DResource* load
    (
        const ObjectIO& io,
        const std::vector<Resource*>& resources
    );
public:
    ~AnimatedTexture2DResource();
    bool set(const std::string& filepath);
    bool set(unsigned char* rawData, unsigned int size);
    bool set(const std::vector<Texture2DResource*>& animationFrames);
    const unsigned int frameId() const {return native_->frameId();}
    void update(const UpdateArgs& args) override;
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
    DECLARE_OVERRIDE_VIRTUALS
};

class FramebufferResource : public Resource
{
    friend Resource;    
    vir::Framebuffer** native_ = nullptr;
    FramebufferResource():Resource(Type::Framebuffer){isNameManaged_=false;}
    
    DELETE_COPY_MOVE(FramebufferResource)

    virtual void save(ObjectIO& io){}
public:
    ~FramebufferResource();
    bool               set(vir::Framebuffer** framebuffer);
    
    virtual void       bind(unsigned int unit) override {(*native_)->bindColorBuffer(unit);unit_ = unit;}
    virtual void       unbind() override {(*native_)->unbind(); unit_ = -1;};
    const unsigned int id() const override {return (*native_)->colorBufferId();}
    const unsigned int width() const override {return (*native_)->width();}
    const unsigned int height() const override{return (*native_)->height();}
    const WrapMode     wrapMode(int index) const override {return (*native_)->colorBufferWrapMode(index);}
    const FilterMode   magFilterMode() const override {return (*native_)->colorBufferMagFilterMode();}
    const FilterMode   minFilterMode() const override {return (*native_)->colorBufferMinFilterMode();}
    virtual void       setWrapMode(int index, WrapMode mode) override {(*native_)->setColorBufferWrapMode(index, mode);}
    virtual void       setMagFilterMode(FilterMode mode) override {(*native_)->setColorBufferMagFilterMode(mode);}
    virtual void       setMinFilterMode(FilterMode mode) override{(*native_)->setColorBufferMinFilterMode(mode);}
};

}