#include "vir/include/vpch.h"
#include "shaderthing/include/oo/resource.h"
#include "shaderthing/include/oo/objectio.h"
#include "shaderthing/include/oo/filedialog.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/helpers.h"

namespace ShaderThing
{

const std::map<Resource::Type, const char*> Resource::typeToName =
{
    {Resource::Type::Texture2D,         "Texture-2D"},
    {Resource::Type::AnimatedTexture2D, "Animation-2D"},
    {Resource::Type::Cubemap,           "Cubemap"},
    {Resource::Type::Texture3D,         "Texture-3D"},
    {Resource::Type::Framebuffer,       "Layer"}
};

Resource::GUI Resource::gui = {};

FileDialog Resource::fileDialog = FileDialog();

//----------------------------------------------------------------------------//

ResourceName::ResourceName()
{
    namePtr_ = new std::string(Helpers::randomString(6));
}

ResourceName::~ResourceName()
{
    if (namePtr_)
        delete namePtr_;
}

std::string ResourceName::name() const
{
    return 
        namePtr_ == nullptr ? 
        (cNamePtr_ == nullptr ? "" : *cNamePtr_) : 
        *namePtr_;
}

void ResourceName::set(const std::string& name)
{
    if (namePtr_)
        delete namePtr_;
    namePtr_ = new std::string(name);
    cNamePtr_ = nullptr;
}

void ResourceName::set(const std::string* namePtr)
{
    if (namePtr_ != nullptr)
        delete namePtr_;
    namePtr_ = nullptr;
    cNamePtr_ = namePtr;
}

//----------------------------------------------------------------------------//

Resource::~Resource()
{
    for (auto& u : clientUniforms_)
        u->deleteValue(true);
}

void Resource::addClientUniform(Uniform* u) 
{
    clientUniforms_.emplace_back(u);
}

void Resource::removeClientUniform(Uniform* u) 
{
    clientUniforms_.erase
    (
        std::remove(clientUniforms_.begin(), clientUniforms_.end(), u), 
        clientUniforms_.end()
    );
}

bool Resource::isUsedByUniform(const Uniform* u) const 
{
    return 
        std::find(clientUniforms_.begin(), clientUniforms_.end(), u) != 
        clientUniforms_.end();
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

template <typename NativeType>
ManagedResource<NativeType>::~ManagedResource()
{
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            this->unbind();
        if (imageUnit_ != -1)
            this->unbindImage();
    }
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

UPtr<Texture2DResource> Texture2DResource::create(const std::string& filepath)
{
    auto resource = UPtr<Texture2DResource>(new Texture2DResource());
    if (resource->set(filepath))
        return resource;
    return vir::nullUniquePtr<Texture2DResource>();
}

UPtr<Texture2DResource> Texture2DResource::create
(
    const unsigned char* rawData, 
    unsigned int size
)
{
    auto resource = UPtr<Texture2DResource>(new Texture2DResource());
    if (resource->set(rawData, size))
        return resource;
    return vir::nullUniquePtr<Texture2DResource>();
}

UPtr<Texture2DResource> Texture2DResource::create
(
    unsigned int width, 
    unsigned int height, 
    InternalFormat internalFormat
)
{
    auto resource = UPtr<Texture2DResource>(new Texture2DResource());
    if (resource->set(width, height, internalFormat))
        return resource;
    return vir::nullUniquePtr<Texture2DResource>();
}

Texture2DResource::~Texture2DResource()
{
    if (rawData_ != nullptr) 
        delete[] rawData_;
}

void Texture2DResource::saveToDisk(ObjectIO& io)
{
    io.writeObjectStart(name().c_str());
    io.write("type", Resource::typeToName.at(type_));
    io.write("magFilterMode", (int)magFilterMode());
    io.write("minFilterMode", (int)minFilterMode());
    io.write("wrapModes", glm::ivec2((int)wrapMode(0), (int)wrapMode(1)));
    io.write("autoUpdateMipmap", autoUpdateMipmap);
    io.write("width", native_->width());
    io.write("height", native_->height());
    io.write("internalFormat", (int)native_->internalFormat());
    if (rawData_ != nullptr)
    {
        io.write("originalFileExtension", originalFileExtension_.c_str());
        io.write("data", (const char*)rawData_, rawDataSize_, true);
    }
    io.writeObjectEnd();
}

UPtr<Texture2DResource> Texture2DResource::load(const ObjectIO& io)
{
    auto resource = UPtr<Texture2DResource>(new Texture2DResource());
    unsigned int rawDataSize;
    if (io.hasMember("data"))
    {
        const char* rawData = io.read("data", true, &rawDataSize);
        resource->set((unsigned char*)rawData, rawDataSize);
        resource->originalFileExtension_ = 
            io.read("originalFileExtension", false);
    }
    else
    {
        unsigned int width, height;
        width = io.read<unsigned int>("width");
        height = io.read<unsigned int>("height");
        InternalFormat internalFormat = 
            (InternalFormat)io.read<unsigned int>("internalFormat");
        resource->set(width, height, internalFormat);
    }
    resource->setName(io.name());
    resource->setMagFilterMode((FilterMode)io.read<int>("magFilterMode"));
    resource->setMinFilterMode((FilterMode)io.read<int>("minFilterMode"));
    auto wrapModes = io.read<glm::ivec2>("wrapModes");
    resource->setWrapMode(0, (WrapMode)wrapModes[0]);
    resource->setWrapMode(1, (WrapMode)wrapModes[1]);
    resource->autoUpdateMipmap = 
        io.readOrDefault<bool>("autoUpdateMipmap", false);
    return resource;
}

bool Texture2DResource::set(const std::string& filepath)
{
    auto native = vir::TextureBuffer2D::create
    (
        filepath, 
        vir::TextureBuffer::InternalFormat::RGBA_UNI_8
    );
    if (native == nullptr)
        return false;
    unsigned int size;
    unsigned char* rawData = Helpers::readFileContents(filepath, size);
    originalFileExtension_ = Helpers::fileExtension(filepath);
    native_ = std::move(native);
    if (rawData_ != nullptr) 
        delete[] rawData_;
    rawData_ = rawData;
    rawDataSize_ = size;
    return true;
}

bool Texture2DResource::set(const unsigned char* rawData, unsigned int size)
{
    auto native = vir::TextureBuffer2D::create
    (
        rawData, 
        size,
        vir::TextureBuffer::defaultInternalFormat(4)
    );
    if (native == nullptr)
        return false;
    native_ = std::move(native);
    if (rawData_ != nullptr) 
        delete[] rawData_;
    rawData_ = rawData;
    rawDataSize_ = size;
    return true;
}

bool Texture2DResource::set
(
    unsigned int width, 
    unsigned int height, 
    InternalFormat internalFormat
)
{
    auto native = vir::TextureBuffer2D::create
    (
        nullptr, 
        width, 
        height, 
        internalFormat
    );
    if (native == nullptr)
        return false;
    native_ = std::move(native);
    if (rawData_ != nullptr) 
        delete[] rawData_;
    rawData_ = nullptr;
    rawDataSize_ = 0;
    return true;
}

void Texture2DResource::update(const UpdateArgs& args)
{
    if (autoUpdateMipmap)
        native_->updateMipmap(true);
}

void Texture2DResource::readData(unsigned char*& data, bool allocate) const
{
    if (native_.valid())
        native_->readData(data, allocate);
}
void Texture2DResource::readData(unsigned int*& data, bool allocate) const
{
    if (native_.valid())
        native_->readData(data, allocate);
}
void Texture2DResource::readData(float*& data, bool allocate) const 
{
    if (native_.valid())
        native_->readData(data, allocate);
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

UPtr<AnimatedTexture2DResource> AnimatedTexture2DResource::create
(
    const std::string& filepath
)
{
    auto resource = UPtr<AnimatedTexture2DResource>
    (
        new AnimatedTexture2DResource()
    );
    if (resource->set(filepath))
        return resource;
    return vir::nullUniquePtr<AnimatedTexture2DResource>();
}

UPtr<AnimatedTexture2DResource> AnimatedTexture2DResource::create
(
    const unsigned char* rawData, 
    unsigned int size
)
{
    auto resource = UPtr<AnimatedTexture2DResource>
    (
        new AnimatedTexture2DResource()
    );
    if (resource->set(rawData, size))
        return resource;
    return vir::nullUniquePtr<AnimatedTexture2DResource>();
}

UPtr<AnimatedTexture2DResource> AnimatedTexture2DResource::create
(
    const std::vector<WPtr<Texture2DResource>>& frames
)
{
    auto resource = UPtr<AnimatedTexture2DResource>
    (
        new AnimatedTexture2DResource()
    );
    if (resource->set(frames))
        return resource;
    return vir::nullUniquePtr<AnimatedTexture2DResource>();
}

AnimatedTexture2DResource::~AnimatedTexture2DResource()
{
    if (rawData_ != nullptr)
        delete[] rawData_;
}

bool AnimatedTexture2DResource::set(const std::string& filepath)
{
    originalFileExtension_ = Helpers::fileExtension(filepath);
    if (originalFileExtension_ != ".gif")
        return false;
    auto native = vir::AnimatedTextureBuffer2D::create
    (
        filepath, 
        vir::TextureBuffer::InternalFormat::RGBA_UNI_8
    );
    if (native == nullptr)
        return false;
    unsigned int size;
    unsigned char* rawData = Helpers::readFileContents(filepath, size);
    native_ = std::move(native);
    if (rawData_ != nullptr) 
        delete[] rawData_;
    rawData_ = rawData;
    rawDataSize_ = size;
    return true;
}

bool AnimatedTexture2DResource::set
(
    const unsigned char* rawData, 
    unsigned int size
)
{
    auto native = vir::AnimatedTextureBuffer2D::create
    (
        rawData,
        size,
        vir::TextureBuffer::defaultInternalFormat(4)
    );
    if (native == nullptr)
        return false;
    native_ = std::move(native);
    if (rawData_ != nullptr) 
        delete[] rawData_;
    rawData_ = rawData;
    rawDataSize_ = size;
    return true;
}

bool AnimatedTexture2DResource::set
(
    const std::vector<WPtr<Texture2DResource>>& frames
)
{
    std::vector<vir::TextureBuffer2D*> nativeFrames;
    for(int i=0; i<(int)frames.size(); i++)
    {
        auto frame = frames[i].get();
        if (!frame)
            continue;
        nativeFrames.emplace_back
        (
            // Forgive me father, for I have sinned
            const_cast<vir::TextureBuffer2D*>(frame->native())
        );
    }
    auto native = vir::AnimatedTextureBuffer2D::create
    (
        nativeFrames,
        false
    );
    if (native == nullptr)
        return false;
    unmanagedFrames_.clear();
    unmanagedFrames_.resize(frames.size());
    for(int i=0; i<(int)frames.size(); i++)
        unmanagedFrames_[i] = frames[i];
    native_ = std::move(native);
    rawDataSize_ = 0;
    return true;
}

void AnimatedTexture2DResource::saveToDisk(ObjectIO& io)
{
    io.writeObjectStart(name().c_str());
    io.write("type", Resource::typeToName.at(type_));
    io.write("magFilterMode", (int)magFilterMode());
    io.write("minFilterMode", (int)minFilterMode());
    io.write("wrapModes", glm::ivec2((int)wrapMode(0), (int)wrapMode(1)));
    io.write("autoUpdateMipmap", autoUpdateMipmap);
    io.write("animationFps", native_->fps());
    io.write("animationFrameIndex", native_->frameIndex());
    io.write("animationPaused", isAnimationPaused);
    io.write("animationBoundToGlobalTime", isAnimationBoundToGlobalTime);
    if // it is a .gif
    (
        originalFileExtension_.size() > 0 && 
        rawData_ != nullptr &&
        rawDataSize_ > 0
    )
    {
        io.write("originalFileExtension", originalFileExtension_.c_str());
        io.write("data", (const char*)rawData_, rawDataSize_, true);
    }
    else // if it is an animation constructed from other resources
    {
        std::vector<std::string> frameNames(unmanagedFrames_.size());
        for (int i=0; i<(int)frameNames.size(); i++)
            frameNames[i] = unmanagedFrames_[i]->name();
        io.write("frames", frameNames);
    }
    io.writeObjectEnd();
}

UPtr<AnimatedTexture2DResource> AnimatedTexture2DResource::load
(
    const ObjectIO& io,
    const std::vector<UPtr<Resource>>& resources
)
{
    auto resource = UPtr<AnimatedTexture2DResource>
    (
        new AnimatedTexture2DResource()
    );
    if (io.hasMember("data"))
    {
        unsigned int rawDataSize;
        const char* rawData = io.read("data", true, &rawDataSize);
        resource->set((unsigned char*)rawData, rawDataSize);
        resource->originalFileExtension_ = 
            io.read("originalFileExtension", false);
    }
    else
    {
        auto frameNames = io.read<std::vector<std::string>>("frames");
        std::vector<WPtr<Texture2DResource>> referencedResources(frameNames.size());
        int i = 0;
        for (auto& frameName : frameNames)
        {
            for (auto& r : resources)
            {
                if 
                (
                    r->name() == frameName && 
                    r->type() == Resource::Type::Texture2D
                )
                    referencedResources[i] = r.getWeakAs<Texture2DResource>();
            }
            ++i;
        }
        resource->set(referencedResources);
    }
    resource->autoUpdateMipmap = 
        io.readOrDefault<bool>("autoUpdateMipmap", false);
    
    resource->native_->setFps(io.read<float>("animationFps"));
    resource->native_->setFrameIndex(io.read<int>("animationFrameIndex"));
    resource->isAnimationPaused = io.read<bool>("animationPaused");
    resource->isAnimationBoundToGlobalTime = 
        io.read<bool>("animationBoundToGlobalTime");

    resource->setName(io.name());
    resource->setMagFilterMode((FilterMode)io.read<int>("magFilterMode"));
    resource->setMinFilterMode((FilterMode)io.read<int>("minFilterMode"));
    auto wrapModes = io.read<glm::ivec2>("wrapModes");
    resource->setWrapMode(0, (WrapMode)wrapModes[0]);
    resource->setWrapMode(1, (WrapMode)wrapModes[1]);
    return resource;
}

void AnimatedTexture2DResource::update(const UpdateArgs& args)
{
    if (isAnimationBoundToGlobalTime)
        native_->setTime(args.time);
    else if (!isAnimationPaused)
        native_->advanceTime(args.timeStep);
    if (autoUpdateMipmap)
        native_->updateMipmap(true);
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

UPtr<CubemapResource> CubemapResource::create
(
    const std::array<WPtr<Texture2DResource>, 6>& faces
)
{
    auto resource = UPtr<CubemapResource>(new CubemapResource());
    if (resource->set(faces))
        return resource;
    return vir::nullUniquePtr<CubemapResource>();
}

bool CubemapResource::set
(
    const std::array<WPtr<Texture2DResource>, 6>& faces
)
{
    const vir::TextureBuffer2D* nativeFaces[6];
    for (int i=0; i<6; i++)
    {
        auto& face = faces[i];
        if (!face.valid()) // At least one Texture2DResouce is invalidated, quit
            return false; 
        nativeFaces[i] = face->native();
    }
    if (!vir::CubeMapBuffer::validFaces(nativeFaces))
        return false;
    const unsigned char* nativeFaceData[6];
    unsigned int size = faces[0]->rawDataSize();
    for (int i=0; i<6; i++)
    {
        auto& face = faces[i];
        nativeFaceData[i] = face->rawData();
        if (face->rawDataSize() != size) // All faces must have the same size
            return false;
    }
    
    auto native = vir::CubeMapBuffer::create
    (
        nativeFaceData, 
        size, 
        vir::TextureBuffer::InternalFormat::RGBA_UNI_8
    );
    if (native == nullptr)
        return false;
    native_ = std::move(native);
    for (int i=0; i<6; i++)
        unmanagedFaces_[i] = faces[i];
    return true;
}

void CubemapResource::saveToDisk(ObjectIO& io)
{
    for (int i=0; i<6; i++)
    {
        if (!unmanagedFaces_[i].valid())
            return;
    }
    io.writeObjectStart(name().c_str());
    io.write("type", Resource::typeToName.at(type_));
    io.write("magFilterMode", (int)magFilterMode());
    io.write("minFilterMode", (int)minFilterMode());
    std::vector<std::string> faceNames(6);
    for (int i=0; i<6; i++)
        faceNames[i] = unmanagedFaces_[i]->name();
    io.write("faces", faceNames);
    io.writeObjectEnd();
}

UPtr<CubemapResource> CubemapResource::load
(
    const ObjectIO& io,
    const std::vector<UPtr<Resource>>& resources
)
{
    auto resource = UPtr<CubemapResource>(new CubemapResource());
    auto faceNames = io.read<std::vector<std::string>>("faces");
    std::array<WPtr<Texture2DResource>, 6> referencedResources;
    int i = 0;
    for (auto& faceName : faceNames)
    {
        for (auto& r : resources)
        {
            if (!r.valid())
                continue;
            if 
            (
                r->name() == faceName && 
                r->type() == Resource::Type::Texture2D
            )
                referencedResources[i] = r.getWeakAs<Texture2DResource>();
        }
        ++i;
    }
    resource->set(referencedResources);

    resource->setName(io.name());
    resource->setMagFilterMode((FilterMode)io.read<int>("magFilterMode"));
    resource->setMinFilterMode((FilterMode)io.read<int>("minFilterMode"));
    return resource;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

UPtr<Texture3DResource> Texture3DResource::create
(
    unsigned int width, 
    unsigned int height, 
    unsigned int depth,
    InternalFormat internalFormat
)
{
    auto resource = UPtr<Texture3DResource>(new Texture3DResource());
    if (resource->set(width, height, depth, internalFormat))
        return resource;
    return vir::nullUniquePtr<Texture3DResource>();
}

bool Texture3DResource::set
(
    unsigned int width, 
    unsigned int height, 
    unsigned int depth, 
    InternalFormat internalFormat
)
{
    auto native = vir::TextureBuffer3D::create
    (
        nullptr, 
        width, 
        height, 
        depth, 
        internalFormat
    );
    if (native == nullptr)
        return false;
    native_ = std::move(native);
    return true;
}

void Texture3DResource::saveToDisk(ObjectIO& io)
{
    io.writeObjectStart(name().c_str());
    io.write("type", Resource::typeToName.at(type_));
    io.write("magFilterMode", (int)magFilterMode());
    io.write("minFilterMode", (int)minFilterMode());
    io.write("wrapModes", glm::ivec3((int)wrapMode(0), (int)wrapMode(1), (int)wrapMode(2)));
    io.write("autoUpdateMipmap", autoUpdateMipmap);
    io.write("width", native_->width());
    io.write("height", native_->height());
    io.write("depth", native_->depth());
    io.write("internalFormat", (int)native_->internalFormat());
    io.writeObjectEnd();
}

UPtr<Texture3DResource> Texture3DResource::load(const ObjectIO& io)
{
    auto resource = UPtr<Texture3DResource>(new Texture3DResource());
    
    unsigned int width, height, depth;
    width = io.read<unsigned int>("width");
    height = io.read<unsigned int>("height");
    depth = io.read<unsigned int>("depth");
    InternalFormat internalFormat = 
        (InternalFormat)io.read<unsigned int>("internalFormat");
    resource->set(width, height, depth, internalFormat);

    resource->setName(io.name());
    resource->setMagFilterMode((FilterMode)io.read<int>("magFilterMode"));
    resource->setMinFilterMode((FilterMode)io.read<int>("minFilterMode"));
    auto wrapModes = io.read<glm::ivec3>("wrapModes");
    resource->setWrapMode(0, (WrapMode)wrapModes[0]);
    resource->setWrapMode(1, (WrapMode)wrapModes[1]);
    resource->setWrapMode(2, (WrapMode)wrapModes[2]);
    resource->autoUpdateMipmap = 
        io.readOrDefault<bool>("autoUpdateMipmap", false);
    return resource;
}

void Texture3DResource::update(const UpdateArgs& args)
{
    if (autoUpdateMipmap)
        native_->updateMipmap(true);
}

void Texture3DResource::readData(unsigned char*& data, bool allocate) const
{
    native_->readData(data, allocate);
}

void Texture3DResource::readData(unsigned int*& data, bool allocate) const
{
    native_->readData(data, allocate);
}

void Texture3DResource::readData(float*& data, bool allocate) const 
{
    native_->readData(data, allocate);
}

//---------------------------------------------------------------------------s-//

//----------------------------------------------------------------------------//

UPtr<LayerResource> LayerResource::create
(
    WPtr<Layer> layer
)
{
    auto resource = UPtr<LayerResource>(new LayerResource());
    if (resource->set(layer))
        return resource;
    return vir::nullUniquePtr<LayerResource>();
}

LayerResource::~LayerResource()
{
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            unbind();
        if (imageUnit_ != -1)
            unbindImage();
    }
}

bool LayerResource::set(WPtr<Layer> layer)
{
    if (!layer.valid() || layer->renderState_.resourceFramebuffer == nullptr)
        return false;
    layer_ = layer;
    native_ = &layer->renderState_.resourceFramebuffer;
    return true;
}

}