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

#include "shaderthing/include/resource.h"

#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/uniform.h"

#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/stb/stb_image_write.h"

namespace ShaderThing
{

FileDialog      Resource::fileDialog_;
const Resource* Resource::resourceToBeExported_ = nullptr;
Resource**      Resource::resourceToBeReplaced_ = nullptr;

std::map<Resource::Type, const char*> Resource::typeToName_ =
{
    {Resource::Type::Texture2D,         "Texture-2D"},
    {Resource::Type::Texture3D,         "Texture-3D"},
    {Resource::Type::AnimatedTexture2D, "Animation-2D"},
    {Resource::Type::Cubemap,           "Cubemap"},
    {Resource::Type::Framebuffer,       "Layer"}
};

void Resource::saveAll(const std::vector<Resource*>& resources, ObjectIO& io)
{
    io.writeObjectStart("resources");
    for (auto* resource : resources)
        resource->save(io);
    io.writeObjectEnd();
}

void Resource::update(std::vector<Resource*>& resources, const UpdateArgs& args)
{
    for (auto* resource : resources)
        resource->update(args);
    if (!Resource::fileDialog_.validSelection())
        return;
    if (resourceToBeExported_ != nullptr) // Export
    {
        auto resource = Resource::resourceToBeExported_;
        auto filepath = fileDialog_.selection().front();
        const unsigned char* rawData = 
            resource->type_ == Resource::Type::Texture2D ?
            ((const Texture2DResource*)resource)->rawData_ :
            ((const AnimatedTexture2DResource*)resource)->rawData_;
        unsigned int rawDataSize = 
            resource->type_ == Resource::Type::Texture2D ?
            ((const Texture2DResource*)resource)->rawDataSize_ :
            ((const AnimatedTexture2DResource*)resource)->rawDataSize_;
        if (rawData != nullptr && rawDataSize > 0)
        {
            std::ofstream file;
            file.open(filepath, std::ios_base::out|std::ios_base::binary);
            if(file.is_open())
            {
                file.write((const char*)rawData, rawDataSize);
                file.close();
            }
        }
        else if (auto texture2D = dynamic_cast<const Texture2DResource*>(resource))
        {
            unsigned char* data = nullptr;
            texture2D->readData(data, true);
            stbi_write_png
            (
                filepath.c_str(), 
                texture2D->width(),
                texture2D->height(),
                texture2D->nChannels(), 
                (const void*)data, 
                texture2D->nChannels()*texture2D->width() // stride
            );
            delete[] data;
        }
        Resource::resourceToBeExported_ = nullptr;
    }
    else if (Resource::resourceToBeReplaced_ != nullptr)
    {
        Resource*& resource = *Resource::resourceToBeReplaced_;
        auto name = resource->name();
        auto filepath = fileDialog_.selection().front();
        auto newResource = Resource::create(filepath);
        if (newResource != nullptr)
        {
            for (auto uniform : resource->clientUniforms_)
            {
                uniform->setValuePtr<const Resource>(newResource);
            }
            if (resource != nullptr)
                delete resource;
            resource = newResource;
            resource->setName(Helpers::filename(filepath));
        }
        Resource::resourceToBeReplaced_ = nullptr;
    }
    else
        for (auto filepath : fileDialog_.selection()) // Load new
        {
            auto newResource = Resource::create(filepath);
            if (newResource != nullptr)
            {
                newResource->setName(Helpers::filename(filepath));
                resources.emplace_back(newResource);
            }
        }
    fileDialog_.clearSelection();
}

Resource::~Resource()
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
}

Resource* Resource::create(const std::string& filepath)
{
    std::string fileExtension = Helpers::fileExtension(filepath);
    if (fileExtension != ".gif")
    {
        auto resource = new Texture2DResource();
        if (resource->set(filepath))
            return resource;
        delete resource;
    }
    else
    {
        auto resource = new AnimatedTexture2DResource();
        if (resource->set(filepath))
            return resource;
        delete resource;
    }
    return nullptr;
}

Resource* Resource::create
(
    const unsigned char* rawData, 
    unsigned int size, 
    bool gif
)
{
    if (!gif)
    {
        auto resource = new Texture2DResource();
        if (resource->set(rawData, size))
            return resource;
        delete resource;
    }
    else
    {
        auto resource = new AnimatedTexture2DResource();
        if (resource->set(rawData, size))
            return resource;
        delete resource;
    }
    return nullptr;
}

Resource* Resource::create
(
    unsigned int width, 
    unsigned int height, 
    InternalFormat internalFormat
)
{
    auto resource = new Texture2DResource();
    if (resource->set(width, height, internalFormat))
        return resource;
    delete resource;
    return nullptr;
}

Resource* Resource::create
(
    unsigned int width, 
    unsigned int height, 
    unsigned int depth,
    InternalFormat internalFormat
)
{
    auto resource = new Texture3DResource();
    if (resource->set(width, height, depth, internalFormat))
        return resource;
    delete resource;
    return nullptr;
}

Resource* Resource::create(const std::vector<Texture2DResource*>& frames)
{
    auto resource = new AnimatedTexture2DResource();
    if (resource->set(frames))
        return resource;
    delete resource;
    return nullptr;
}

Resource* Resource::create(const Texture2DResource* faces[6])
{
    auto resource = new CubemapResource();
    if (resource->set(faces))
        return resource;
    delete resource;
    return nullptr;
}

Resource* Resource::create(Layer* layer)
{
    auto resource = new LayerResource();
    if (resource->set(layer))
        return resource;
    delete resource;
    return nullptr;
}

void Resource::setName(const std::string& name)
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
    namePtr_ = new std::string(name);
}

void Resource::setNamePtr(std::string* namePtr)
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
    namePtr_ = namePtr;
}

void Resource::loadAll
(
    const ObjectIO& io,
    std::vector<Resource*>& resources
)
{
    for (auto resource : resources)
    {
        DELETE_IF_NOT_NULLPTR(resource)
    }
    resources.clear();
    auto ioType = [](const ObjectIO& io)
    {
        auto typeName = io.read("type", false);
        for (auto entry : typeToName_)
        {
            if (std::string(typeName) != std::string(entry.second))
                continue;
            return entry.first;
        }
        return Type();
    };
    auto ioResources = io.readObject("resources");

    // First, load all resources of type Texture2D as these might be referenced
    // by AnimatedTexture2D or Cubemap resources
    for (auto ioResourceName : ioResources.members())
    {
        auto ioResource = ioResources.readObject(ioResourceName);
        auto type = ioType(ioResource);
        if (type != Type::Texture2D)
            continue;
        auto resource = Texture2DResource::load(ioResource);
        if (resource != nullptr)
            resources.emplace_back(resource);
    }
    // Then load all the remaining resources
    for (auto ioResourceName : ioResources.members())
    {
        auto ioResource = ioResources.readObject(ioResourceName);
        auto type = ioType(ioResource);
        Resource* resource = nullptr;
        switch (type)
        {
            default :
                continue;
            case Type::Texture3D :
                resource = 
                    Texture3DResource::load(ioResource);
                break;
            case Type::AnimatedTexture2D :
                resource = 
                    AnimatedTexture2DResource::load(ioResource, resources);
                break;
            case Type::Cubemap :
                resource = 
                    CubemapResource::load(ioResource, resources);
                break;
        }
        if (resource != nullptr)
            resources.emplace_back(resource);
    }
}

void Resource::resetAnimationsTime
(
    const std::vector<Resource*>& resources, 
    float time
)
{
    for (auto resource : resources)
    {
        if (resource->type() != Resource::Type::AnimatedTexture2D)
            continue;
        auto animation = (AnimatedTexture2DResource*)resource;
        if (!animation->isAnimationBoundToGlobalTime_)
            continue;
        if (animation->isAnimationPaused_)
            continue;
        animation->native_->setTime(time);
    }
}

void Resource::prepareAnimationsForExport
(
    const std::vector<Resource*>& resources,
    float startTime, 
    bool forceResumeTime,
    bool cacheTime
)
{
    for (auto resource : resources)
    {
        if (resource->type() != Resource::Type::AnimatedTexture2D)
            continue;
        auto animation = (AnimatedTexture2DResource*)resource;
        if (!animation->isAnimationBoundToGlobalTime_)
            continue;
        if (animation->isAnimationPaused_ && !forceResumeTime)
            continue;
        if (cacheTime)
            animation->cachedTime_ = animation->native_->time();
        if (forceResumeTime)
            animation->native_->setTime(startTime);
    }
}

void Resource::resetAnimationsAfterExport
(
    const std::vector<Resource*>& resources
)
{
    for (auto resource : resources)
    {
        if (resource->type() != Resource::Type::AnimatedTexture2D)
            continue;
        auto animation = (AnimatedTexture2DResource*)resource;
        if (!animation->isAnimationBoundToGlobalTime_)
            continue;
        if (animation->isAnimationPaused_)
            continue;
        animation->native_->setTime(animation->cachedTime_);
    }
}

//----------------------------------------------------------------------------//

#define SET_NATIVE_AND_RAW_AND_RETURN(data, size)                           \
    if (native_ != nullptr) delete native_;                                 \
    native_ = native;                                                       \
    if (rawData_ != nullptr) delete[] rawData_;                             \
    rawData_ = data;                                                        \
    rawDataSize_ = size;                                                    \
    return true;

bool Texture2DResource::set(const std::string& filepath)
{
    auto native = vir::TextureBuffer2D::create
    (
        filepath, 
        vir::TextureBuffer::InternalFormat::RGBA_UNI_8
    );
    if (native == nullptr)
        return false;
    unsigned int rawDataSize;
    unsigned char* rawData = Helpers::readFileContents(filepath, rawDataSize);
    originalFileExtension_ = Helpers::fileExtension(filepath);
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, rawDataSize)
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
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, size)
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
    SET_NATIVE_AND_RAW_AND_RETURN(nullptr, 0)
}

Texture2DResource::~Texture2DResource()
{
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            this->unbind();
        if (imageUnit_ != -1)
            this->unbindImage();
        delete native_;
    }
    if (rawData_ != nullptr) 
        delete[] rawData_;
}

void Texture2DResource::save(ObjectIO& io)
{
    io.writeObjectStart(namePtr_->c_str());
    io.write("type", Resource::typeToName_[type_]);
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

Texture2DResource* Texture2DResource::load(const ObjectIO& io)
{
    auto resource = new Texture2DResource();
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

void Texture2DResource::update(const UpdateArgs& args)
{
    if (autoUpdateMipmap)
        native_->updateMipmap(true);
}

//----------------------------------------------------------------------------//

bool AnimatedTexture2DResource::set(const std::string& filepath)
{
    // Create native resource from raw data
    auto native = vir::AnimatedTextureBuffer2D::create
    (
        filepath, 
        vir::TextureBuffer::InternalFormat::RGBA_UNI_8
    );
    if (native == nullptr)
        return false;
    unsigned int rawDataSize;
    unsigned char* rawData = Helpers::readFileContents(filepath, rawDataSize);
    originalFileExtension_ = Helpers::fileExtension(filepath);
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, rawDataSize)
}

bool AnimatedTexture2DResource::set
(
    const unsigned char* rawData, 
    unsigned int size
)
{
    vir::AnimatedTextureBuffer2D* native = nullptr;
    native = vir::AnimatedTextureBuffer2D::create
    (
        rawData,
        size,
        vir::TextureBuffer::defaultInternalFormat(4)
    );
    if (native == nullptr)
        return false;
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, size)
}

bool AnimatedTexture2DResource::set
(
    const std::vector<Texture2DResource*>& frames
)
{
    std::vector<vir::TextureBuffer2D*> nativeFrames(frames.size());
    for(int i=0; i<(int)frames.size(); i++)
        nativeFrames[i] = frames[i]->native_;
    vir::AnimatedTextureBuffer2D* native = nullptr;
    native = vir::AnimatedTextureBuffer2D::create
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
    SET_NATIVE_AND_RAW_AND_RETURN(nullptr, 0)
}

AnimatedTexture2DResource::~AnimatedTexture2DResource()
{
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            this->unbind();
        if (imageUnit_ != -1)
            this->unbindImage();
        delete native_;
    }
    if (rawData_ != nullptr)
        delete[] rawData_;
}

void AnimatedTexture2DResource::save(ObjectIO& io)
{
    io.writeObjectStart(namePtr_->c_str());
    io.write("type", Resource::typeToName_[type_]);
    io.write("magFilterMode", (int)magFilterMode());
    io.write("minFilterMode", (int)minFilterMode());
    io.write("wrapModes", glm::ivec2((int)wrapMode(0), (int)wrapMode(1)));
    io.write("autoUpdateMipmap", autoUpdateMipmap);
    io.write("animationFps", native_->fps());
    io.write("animationFrameIndex", native_->frameIndex());
    io.write("animationPaused", isAnimationPaused_);
    io.write("animationBoundToGlobalTime", isAnimationBoundToGlobalTime_);
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

void AnimatedTexture2DResource::update(const UpdateArgs& args)
{
    if (isAnimationBoundToGlobalTime_)
        native_->setTime(args.time);
    else if (!isAnimationPaused_)
        native_->advanceTime(args.timeStep);
    if (autoUpdateMipmap)
        native_->updateMipmap(true);
}

AnimatedTexture2DResource* AnimatedTexture2DResource::load
(
    const ObjectIO& io, 
    const std::vector<Resource*>& resources
)
{
    auto resource = new AnimatedTexture2DResource();
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
        std::vector<Texture2DResource*> referencedResources(frameNames.size());
        int i = 0;
        for (auto& frameName : frameNames)
        {
            for (auto r : resources)
            {
                if 
                (
                    r->name() == frameName && 
                    r->type() == Resource::Type::Texture2D
                )
                    referencedResources[i] = (Texture2DResource*)r;
            }
            ++i;
        }
        resource->set(referencedResources);
    }
    resource->autoUpdateMipmap = 
        io.readOrDefault<bool>("autoUpdateMipmap", false);
    
    resource->native_->setFps(io.read<float>("animationFps"));
    resource->native_->setFrameIndex(io.read<int>("animationFrameIndex"));
    resource->isAnimationPaused_ = io.read<bool>("animationPaused");
    resource->isAnimationBoundToGlobalTime_ = 
        io.read<bool>("animationBoundToGlobalTime");

    resource->setName(io.name());
    resource->setMagFilterMode((FilterMode)io.read<int>("magFilterMode"));
    resource->setMinFilterMode((FilterMode)io.read<int>("minFilterMode"));
    auto wrapModes = io.read<glm::ivec2>("wrapModes");
    resource->setWrapMode(0, (WrapMode)wrapModes[0]);
    resource->setWrapMode(1, (WrapMode)wrapModes[1]);
    return resource;
}

//----------------------------------------------------------------------------//

bool CubemapResource::set(const Texture2DResource* faces[6])
{
    const vir::TextureBuffer2D* nativeFaces[6];
    for (int i=0; i<6; i++)
        nativeFaces[i] = (faces[i])->native_;
    if (!vir::CubeMapBuffer::validFaces(nativeFaces))
        return false;
    const unsigned char* nativeFaceData[6];
    for (int i=0; i<6; i++)
        nativeFaceData[i] = (faces[i])->rawData_;
    unsigned int size = faces[0]->rawDataSize_;
    
    auto native = vir::CubeMapBuffer::create
    (
        nativeFaceData, 
        size, 
        vir::TextureBuffer::InternalFormat::RGBA_UNI_8
    );
    if (native == nullptr)
        return false;
    if (native_ != nullptr) 
        delete native_;
    native_ = native;
    for (int i=0; i<6; i++)
        unmanagedFaces_[i] = faces[i];
    return true;
}

CubemapResource::~CubemapResource()
{
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            this->unbind();
        if (imageUnit_ != -1)
            this->unbindImage();
        delete native_;
    }
}

void CubemapResource::save(ObjectIO& io)
{
    io.writeObjectStart(namePtr_->c_str());
    io.write("type", Resource::typeToName_[type_]);
    io.write("magFilterMode", (int)magFilterMode());
    io.write("minFilterMode", (int)minFilterMode());
    std::vector<std::string> faceNames(6);
    for (int i=0; i<6; i++)
        faceNames[i] = unmanagedFaces_[i]->name();
    io.write("faces", faceNames);
    io.writeObjectEnd();
}

CubemapResource* CubemapResource::load
(
    const ObjectIO& io,
    const std::vector<Resource*>& resources
)
{
    auto resource = new CubemapResource();
    auto faceNames = io.read<std::vector<std::string>>("faces");
    const Texture2DResource* referencedResources[6];
    int i = 0;
    for (auto& faceName : faceNames)
    {
        for (auto r : resources)
        {
            if 
            (
                r->name() == faceName && 
                r->type() == Resource::Type::Texture2D
            )
                referencedResources[i] = (Texture2DResource*)r;
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
    if (native_ != nullptr) 
        delete native_;
    native_ = native;
    return true;
}

Texture3DResource::~Texture3DResource()
{
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            this->unbind();
        if (imageUnit_ != -1)
            this->unbindImage();
        delete native_;
    }
}

void Texture3DResource::save(ObjectIO& io)
{
    io.writeObjectStart(namePtr_->c_str());
    io.write("type", Resource::typeToName_[type_]);
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

Texture3DResource* Texture3DResource::load(const ObjectIO& io)
{
    auto resource = new Texture3DResource();
    
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

//----------------------------------------------------------------------------//

bool LayerResource::set(Layer* layer)
{
    if (layer == nullptr || layer->rendering_.resourceFramebuffer== nullptr)
        return false;
    layer_ = layer;
    native_ = &layer->rendering_.resourceFramebuffer;
    return true;
}

LayerResource::~LayerResource()
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
//----------------------------------------------------------------------------//

bool Resource::isGuiOpen = false;
bool Resource::isGuiDetachedFromMenu = true;

void Resource::renderResourcesGui
(
    std::vector<Resource*>& resources,
    const std::vector<Layer*>& layers
)
{
    if (!Resource::isGuiOpen)
        return;
    if (Resource::isGuiDetachedFromMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(900,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Resource manager", &Resource::isGuiOpen, windowFlags);

        // Refresh icon if needed
        static bool isIconSet(false);
        static bool isWindowDocked(ImGui::IsWindowDocked());
        if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
        {
            isIconSet = vir::ImGuiRenderer::setWindowIcon
            (
                "Resource manager", 
                ByteData::Icon::sTIconData, 
                ByteData::Icon::sTIconSize,
                false
            );
            isWindowDocked = ImGui::IsWindowDocked();
        }
    }

    const float fontSize = ImGui::GetFontSize();
    const float buttonWidth(12*fontSize);
    const float cursorPosY0 = ImGui::GetCursorPosY();

    #define START_ROW                                                       \
        ImGui::PushID(row);                                                 \
        ImGui::TableNextRow(0, 1.6*fontSize);                               \
        column = 0;
    #define END_ROW                                                         \
        ImGui::PopID();
    #define START_COLUMN                                                    \
        ImGui::TableSetColumnIndex(column);                                 \
        ImGui::PushItemWidth(-1);
    #define END_COLUMN                                                      \
        ++column;                                                           \
        ImGui::PopItemWidth();
    #define NEXT_COLUMN                                                     \
        ImGui::TableSetColumnIndex(column++);

    //--------------------------------------------------------------------------
    auto renderResourceGui = 
    [
        &fontSize, 
        &buttonWidth
    ]
    (
        bool& deleteResource,
        std::vector<Resource*>& resources,
        const int row
    )
    {
        int column = 0;
        START_ROW
        START_COLUMN // Actions column -----------------------------------------
        renderResourceActionsButtonGui
        (
            resources[row], 
            deleteResource, 
            resources, 
            ImVec2(buttonWidth, 0)
        );
        Resource* resource = resources[row];
        END_COLUMN
        START_COLUMN // Type column --------------------------------------------
        std::string typeName = typeToName_.at(resource->type_);
        if (auto texture = dynamic_cast<Texture2DResource*>(resource))
        {
            // Small fix to distinguish storage textures from "regular" image-
            // loaded textures. I will need to revist the whole storage-texture
            // thing in the future, or add the same functionalities (namely
            // resizing/reformatting) to "regular" image-loaded textures to 
            // remove such somewhat artificial distinctions altogether
            if (!texture->hasRawData())
                typeName += " (S)";
        }
        ImGui::Text(typeName.c_str());
        END_COLUMN
        START_COLUMN // Preview column -----------------------------------------
        float x = resource->width();
        float y = resource->height();
        float aspectRatio = x/y;
        auto previewTexture2D = 
        [&aspectRatio]
        (
            const Resource* resource, 
            float sideSize, 
            float offset
        )->void
        {
            ImVec2 previewSize;
            ImVec2 hoverSize{256,256};
            if (aspectRatio > 1.0)
            {
                previewSize = ImVec2(sideSize, sideSize/aspectRatio);
                hoverSize.y /= aspectRatio;
            }
            else
            { 
                previewSize = ImVec2(sideSize*aspectRatio, sideSize);
                hoverSize.x *= aspectRatio;
            }
            float startx = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(startx + offset);
#define SHOW_IMAGE(size)                                                    \
    ImGui::Image                                                            \
    (                                                                       \
        (ImTextureID)                                                       \
        (                                                                   \
            resource->type_ != Resource::Type::AnimatedTexture2D ?          \
            resource->id() :                                                \
            ((AnimatedTexture2DResource*)resource)->frameId()               \
        ),                                                                  \
        size,                                                               \
        {0,1},                                                              \
        {1,0}                                                               \
    );
            SHOW_IMAGE(previewSize)
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                SHOW_IMAGE(hoverSize)
                ImGui::EndTooltip();
            }
        };
        if (resource->isInternalFormatUnsigned())
            ImGui::Text("N/A");
        else if 
        (
            resource->type_ == Resource::Type::Texture2D ||
            resource->type_ == Resource::Type::AnimatedTexture2D ||
            resource->type_ == Resource::Type::Framebuffer
        )
            previewTexture2D(resource, 1.4*fontSize, 1.3*fontSize);
        else if (resource->type_ == Resource::Type::Cubemap)
        {
            auto cubemap = (CubemapResource*)resource;
            for (int i=0; i<6; i++)
            {
                float offset = (i == 0 || i == 3) ? 0.75*fontSize : 0.0;
                previewTexture2D
                (
                    cubemap->unmanagedFaces_[i], 
                    0.5*fontSize, 
                    offset
                );
                if ((i+1)%3 != 0)
                    ImGui::SameLine();
            }
        }
        END_COLUMN
        START_COLUMN // Name column --------------------------------------------
        if (resource->type_ != Type::Framebuffer)
        {
            if (ImGui::InputText("##resourceName", resource->namePtr_))
                Helpers::enforceUniqueName
                (
                    *(resource->namePtr_),
                    resources,
                    resource
                );
        }
        else
            ImGui::Text(resource->name().c_str());
        END_COLUMN
        START_COLUMN // Resolution column --------------------------------------
        if (resource->type_ == Type::Texture3D)
            ImGui::Text("%d x %d x %d", (int)x, (int)y, (int)resource->depth());
        else
            ImGui::Text("%d x %d", (int)x, (int)y);
        END_COLUMN
        START_COLUMN // Aspect ratio column ------------------------------------
        ImGui::Text("%.3f", aspectRatio);
        END_COLUMN
        END_ROW
    }; // End of renderResourceGui lambda

    //--------------------------------------------------------------------------
    auto renderAddResourceButtonGui = [&fontSize, &buttonWidth]
    (
        std::vector<Resource*>& resources,
        const int row
    )
    {
        int column = 0;
        START_ROW
        NEXT_COLUMN
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1,0)))
            ImGui::OpenPopup("##addResourcePopup");
        if (ImGui::BeginPopup("##addResourcePopup"))
        {
            Resource* resource = nullptr;
            Resource::loadOrReplaceTexture2DOrAnimationButtonGui
            (
                resource,
                ImVec2(buttonWidth, 0),
                false
            );
            Resource::loadOrReplaceTexture2DOrAnimationButtonGui
            (
                resource,
                ImVec2(buttonWidth, 0),
                true
            );
            createOrResizeOrReformatTexture2DGui
            (
                resource,
                true,
                false,
                ImVec2(buttonWidth, 0)
            );
            createOrResizeOrReformatTexture3DGui
            (
                resource,
                true,
                false,
                ImVec2(buttonWidth, 0)
            );
            Resource::createOrEditAnimationButtonGui
            (
                resource,
                resources,
                ImVec2(buttonWidth, 0)
            );
            Resource::createOrEditCubemapButtonGui
            (
                resource,
                resources,
                ImVec2(buttonWidth, 0)
            );
            if (resource != nullptr)
            {
                if (resource->name().size() == 0)
                    resource->setName(Helpers::randomString(6));
                Helpers::enforceUniqueName
                (
                    *(resource->namePtr_),
                    resources
                );
                resources.emplace_back(resource);
            }
            ImGui::EndPopup();
        }
        END_ROW
    }; // End of renderAddResourceButtonGui lambda
    
    //--------------------------------------------------------------------------
    static float tableHeight = 0;
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##resourceTable", 6, flags, ImVec2(0., tableHeight)))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("##controls", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Type", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Preview", flags, 4.0*fontSize);
        ImGui::TableSetupColumn("Name", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Resolution", flags, 10.0*fontSize);
        ImGui::TableSetupColumn
        (
            "Aspect ratio", 
            flags, 
            Resource::isGuiDetachedFromMenu ? 
            ImGui::GetContentRegionAvail().x : 8.0*fontSize
        );
        ImGui::TableHeadersRow();

        int deleteRow = -1;
        bool deleteResource = false;
        const int nRows = resources.size();
        for (int row=0; row<nRows; row++)
        {
            renderResourceGui(deleteResource, resources, row);
            if (deleteResource)
            {
                deleteRow = row;
                deleteResource = false;
            }
        }
        renderAddResourceButtonGui(resources, nRows);
        tableHeight = (ImGui::GetCursorPosY()-cursorPosY0);
        if (deleteRow != -1)
        {
            auto resource = resources[deleteRow];
            for (auto layer : layers)
                layer->removeResourceFromUniforms(resource);
            resources.erase(resources.begin()+deleteRow);
            delete resource;
        }
        ImGui::EndTable();
    }

    if (Resource::isGuiDetachedFromMenu)
        ImGui::End();
}

//----------------------------------------------------------------------------//

void Resource::renderResourcesMenuItemGui
(
    std::vector<Resource*>& resources,
    const std::vector<Layer*>& layers
)
{
    if 
    (
        ImGui::SmallButton
        (
            isGuiDetachedFromMenu ? 
            ICON_FA_WINDOW_MAXIMIZE : 
            ICON_FA_ARROW_RIGHT
        )
    )
        isGuiDetachedFromMenu = !isGuiDetachedFromMenu;
    ImGui::SameLine();
    if (!isGuiDetachedFromMenu)
    {
        if (ImGui::BeginMenu("Resource manager"))
        {
            isGuiOpen = true;
            Resource::renderResourcesGui(resources, layers);
            ImGui::EndMenu();
        }
        else
            isGuiOpen = false;
        return;
    }
    ImGui::MenuItem("Resource manager", NULL, &Resource::isGuiOpen);
}

bool LayerResource::insertInResources
(
    Layer* layer,
    std::vector<Resource*>& resources
)
{
    for (int i=0; i<(int)resources.size(); i++)
    {
        auto resource = resources[i];
        if (resource->type() != Type::Framebuffer)
            continue;
        if (resource->name() == layer->name())
            return false;
    }
    auto resource = resources.emplace_back(Resource::create(layer));
    resource->setNamePtr(&(layer->gui_.name));
    return true;
}

bool LayerResource::removeFromResources
(
    const Layer* layer,
    std::vector<Resource*>& resources
)
{
    for (int i=0; i<(int)resources.size(); i++)
    {
        auto resource = resources[i];
        if (resource->type() != Type::Framebuffer)
            continue;
        if (resource->name() == layer->name())
        {
            resources.erase(resources.begin()+i);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------//

bool Resource::loadOrReplaceTexture2DOrAnimationButtonGui
(
    Resource*& resource,
    const ImVec2 size,
    const bool animation,
    const bool disabled
)
{
    if (disabled)
        ImGui::BeginDisabled();
    if 
    (
        ImGui::Button
        (
            resource == nullptr ? 
            (animation ? "Load animation" : "Load texture-2D") : 
            "Replace",
            size
        )
    )
    {
        animation ?
        Resource::fileDialog_.runOpenFileDialog
        (
            "Select a GIF", {"GIFs (.gif)", "*.gif"}, ".", (resource == nullptr)
        ) :
        Resource::fileDialog_.runOpenFileDialog
        (
            "Select an image",
            {
                "Image files (.png,.jpg,.jpeg,.bmp)", 
                "*.png *.jpg *.jpeg *.bmp"
            },
            ".",
            (resource == nullptr)
        );
        if (resource != nullptr)
            Resource::resourceToBeReplaced_ = &resource;
        return true;
    }
    if (disabled)
        ImGui::EndDisabled();
    return false;
}

//----------------------------------------------------------------------------//

bool Resource::createOrResizeOrReformatTexture2DGui
(
    Resource*& resource,
    const bool enablePopup,
    const bool resetValues,
    const ImVec2 buttonSize
)
{
    bool valid = false;
    static glm::ivec2 resolution(1, 1);
    static InternalFormat internalFormat = InternalFormat::RGBA_SF_32;
    if (enablePopup && ImGui::Button("Create texture-2D", buttonSize))
    {
        ImGui::OpenPopup("##createTexturePopup");
        resolution = {1,1};
        internalFormat = InternalFormat::RGBA_SF_32;
    }
    if (resetValues && resource != nullptr)
    {
        resolution.x = resource->width();
        resolution.y = resource->height();
        internalFormat = ((Texture2DResource*)resource)->internalFormat();
    }
    if (ImGui::BeginPopup("##createTexturePopup") || !enablePopup)
    {
        if (resource == nullptr)
        {
            ImGui::Text(
R"(Create a blank texture, useful for e.g., 
shader data storage via imageLoad and
imageStore operations. Data written to 
these textures will not be saved within 
the project)");
            ImGui::Separator();
        }
        if (enablePopup)
            ImGui::Text("Resolution ");
        else
            ImGui::Text("Resolution          ");
        ImGui::SameLine();
        float itemWidth = -1; //resource == nullptr ? -1 : 12*ImGui::GetFontSize();
        ImGui::PushItemWidth(itemWidth);
        if 
        (
            ImGui::InputInt2
            (
                "##windowResolution", 
                glm::value_ptr(resolution)
            )
        )
        {
            resolution.x = std::min(std::max(resolution.x, 1), 4096);
            resolution.y = std::min(std::max(resolution.y, 1), 4096);
        }
        ImGui::PopItemWidth();
        if (enablePopup)
            ImGui::Text("Format     ");
        else
            ImGui::Text("Format              ");
        ImGui::SameLine();
        ImGui::PushItemWidth(itemWidth);
        // RGB formats are not easy to work with due to memory-alignment 
        // limitations so they are omitted
        static std::vector<InternalFormat> supportedFormats = 
        {
            InternalFormat::R_UI_32,
            InternalFormat::R_SF_32,
            InternalFormat::RG_UI_32,
            InternalFormat::RG_SF_32,
            InternalFormat::RGBA_UI_32,
            InternalFormat::RGBA_SF_32
        };
        if
        (
            ImGui::BeginCombo
            (
                "##textureFormatCombo",
                vir::TextureBuffer2D::internalFormatToName.at
                (
                    internalFormat
                ).c_str()
            )
        )
        {
            for (auto format : supportedFormats)
            {
                if 
                (
                    ImGui::Selectable
                    (
                        vir::TextureBuffer2D::internalFormatToName.at
                        (
                            format
                        ).c_str()
                    )
                )
                {
                    internalFormat = format;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        bool disabled = false;
        if (resource != nullptr)
        {
            ImGui::Text("VRAM footprint      ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text("VRAM memory occupied by this texture");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            double maxMemoryFootprint = resource->maxMemoryFootprint();
            auto uom = Helpers::autoRescaleMemoryValue(maxMemoryFootprint);
            ImGui::Text("%.1f %s", maxMemoryFootprint, uom);
            
            if 
            (
                (int)resource->width() == resolution.x && 
                (int)resource->height() == resolution.y && 
                resource->internalFormat() == internalFormat
            )
            {
                ImGui::BeginDisabled();
                disabled = true;
            }
        }
        if 
        (
            ImGui::Button
            (
                resource == nullptr ? "Create texture-2D" : "Resize or reformat", 
                ImVec2(-1, 0)
            )
        )
        {
            if (resource != nullptr)
            {
                auto wrapMode0 = resource->wrapMode(0);
                auto wrapMode1 = resource->wrapMode(1);
                auto minFilterMode = resource->minFilterMode();
                auto magFilterMode = resource->magFilterMode();
                auto format0 = ((Texture2DResource*)resource)->internalFormat();
                if 
                (
                    ((Texture2DResource*)resource)->set
                    (
                        resolution.x, 
                        resolution.y, 
                        internalFormat
                    )
                )
                {
                    valid = true;
                    auto format = 
                        ((Texture2DResource*)resource)->internalFormat();
                    resource->setWrapMode(0, wrapMode0);
                    resource->setWrapMode(1, wrapMode1);
                    resource->setMinFilterMode(minFilterMode);
                    resource->setMagFilterMode(magFilterMode);
                    if (format != format0)
                    {
                        if (resource->clientUniforms_.size() > 0)
                            Layer::Flags::requestRecompilation = true;
                    }
                }
            }
            else
            {
                auto newResource = 
                    Resource::create
                    (
                        resolution.x,
                        resolution.y,
                        internalFormat
                    );
                if (newResource != nullptr)
                {
                    resource = newResource;
                    valid = true;
                }
            }
            resolution.x = resource->width();
            resolution.y = resource->height();
            internalFormat = ((Texture2DResource*)resource)->internalFormat();
        }
        if (!disabled || resource == nullptr)
        {
            createOrResizeOrReformatTextureMemoryEstimateGui
            (
                uint64_t(resolution.x)*uint64_t(resolution.y),
                internalFormat,
                true
            );
        }
        if (disabled)
            ImGui::EndDisabled();
        if (enablePopup)
            ImGui::EndPopup();
    }
    return valid;
}

//----------------------------------------------------------------------------//

bool Resource::createOrResizeOrReformatTexture3DGui
(
    Resource*& resource,
    const bool enablePopup,
    const bool resetValues,
    const ImVec2 buttonSize
)
{
    bool valid = false;
    static glm::ivec3 resolution(1, 1, 1);
    static InternalFormat internalFormat = InternalFormat::RGBA_SF_32;
    if (enablePopup && ImGui::Button("Create texture-3D", buttonSize))
    {
        ImGui::OpenPopup("##createTexture3DPopup");
        resolution = {1,1,1};
        internalFormat = InternalFormat::RGBA_SF_32;
    }
    if (resetValues && resource != nullptr)
    {
        resolution.x = resource->width();
        resolution.y = resource->height();
        resolution.z = resource->height();
        internalFormat = ((Texture3DResource*)resource)->internalFormat();
    }
    if (ImGui::BeginPopup("##createTexture3DPopup") || !enablePopup)
    {
        if (resource == nullptr)
        {
            ImGui::Text(
R"(Create a blank texture, useful for e.g., 
shader data storage via imageLoad and
imageStore operations. Data written to 
these textures will not be saved within 
the project)");
            ImGui::Separator();
        }
        if (enablePopup)
            ImGui::Text("Resolution ");
        else
            ImGui::Text("Resolution          ");
        ImGui::SameLine();
        float itemWidth = -1;
        ImGui::PushItemWidth(itemWidth);
        if 
        (
            ImGui::InputInt3
            (
                "##windowResolution", 
                glm::value_ptr(resolution)
            )
        )
        {
            int maxSize = std::min(vir::TextureBuffer3D::maxSideSize(), 2048u);
            resolution.x = std::min(std::max(resolution.x, 1), maxSize);
            resolution.y = std::min(std::max(resolution.y, 1), maxSize);
            resolution.z = std::min(std::max(resolution.z, 1), maxSize);
        }
        ImGui::PopItemWidth();
        if (enablePopup)
            ImGui::Text("Format     ");
        else
            ImGui::Text("Format              ");
        ImGui::SameLine();
        ImGui::PushItemWidth(itemWidth);
        // RGB formats are not easy to work with due to memory-alignment 
        // limitations so they are omitted
        static std::vector<InternalFormat> supportedFormats = 
        {
            InternalFormat::R_UNI_8,
            InternalFormat::R_UI_32,
            InternalFormat::R_SF_32,
            InternalFormat::RG_UNI_8,
            InternalFormat::RG_UI_32,
            InternalFormat::RG_SF_32,
            InternalFormat::RGBA_UNI_8,
            InternalFormat::RGBA_UI_32,
            InternalFormat::RGBA_SF_32
        };
        if
        (
            ImGui::BeginCombo
            (
                "##textureFormatCombo",
                vir::TextureBuffer::internalFormatToName.at
                (
                    internalFormat
                ).c_str()
            )
        )
        {
            for (auto format : supportedFormats)
            {
                if 
                (
                    ImGui::Selectable
                    (
                        vir::TextureBuffer::internalFormatToName.at
                        (
                            format
                        ).c_str()
                    )
                )
                {
                    internalFormat = format;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        bool disabled = false;
        if (resource != nullptr)
        {
            ImGui::Text("VRAM footprint      ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text("VRAM memory occupied by this texture");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            double maxMemoryFootprint = resource->maxMemoryFootprint();
            auto uom = Helpers::autoRescaleMemoryValue(maxMemoryFootprint);
            ImGui::Text("%.1f %s", maxMemoryFootprint, uom);

            if 
            (
                (int)resource->width() == resolution.x && 
                (int)resource->height() == resolution.y && 
                (int)resource->depth() == resolution.z && 
                resource->internalFormat() == internalFormat
            )
            {
                ImGui::BeginDisabled();
                disabled = true;
            }
        }
        if 
        (
            ImGui::Button
            (
                resource == nullptr ? "Create texture-3D" : "Resize or reformat", 
                ImVec2(-1, 0)
            )
        )
        {
            if (resource != nullptr)
            {
                auto wrapMode0 = resource->wrapMode(0);
                auto wrapMode1 = resource->wrapMode(1);
                auto wrapMode2 = resource->wrapMode(2);
                auto minFilterMode = resource->minFilterMode();
                auto magFilterMode = resource->magFilterMode();
                auto format0 = ((Texture3DResource*)resource)->internalFormat();
                if 
                (
                    ((Texture3DResource*)resource)->set
                    (
                        resolution.x, 
                        resolution.y,
                        resolution.z,
                        internalFormat
                    )
                )
                {
                    valid = true;
                    auto format = 
                        ((Texture3DResource*)resource)->internalFormat();
                    resource->setWrapMode(0, wrapMode0);
                    resource->setWrapMode(1, wrapMode1);
                    resource->setWrapMode(2, wrapMode2);
                    resource->setMinFilterMode(minFilterMode);
                    resource->setMagFilterMode(magFilterMode);
                    if (format != format0)
                    {
                        if (resource->clientUniforms_.size() > 0)
                            Layer::Flags::requestRecompilation = true;
                    }
                }
            }
            else
            {
                auto newResource = 
                    Resource::create
                    (
                        resolution.x,
                        resolution.y,
                        resolution.z,
                        internalFormat
                    );
                if (newResource != nullptr)
                {
                    resource = newResource;
                    valid = true;
                }
            }
            resolution.x = resource->width();
            resolution.y = resource->height();
            resolution.z = resource->depth();
            internalFormat = ((Texture3DResource*)resource)->internalFormat();
        }
        if (!disabled || resource == nullptr)
        {
            createOrResizeOrReformatTextureMemoryEstimateGui
            (
                uint64_t(resolution.x)*uint64_t(resolution.y)*uint64_t(resolution.z),
                internalFormat,
                false
            );
        }
        if (disabled)
            ImGui::EndDisabled();
        if (enablePopup)
            ImGui::EndPopup();
    }
    return valid;
}

void Resource::createOrResizeOrReformatTextureMemoryEstimateGui
(
    uint64_t textureSize,
    InternalFormat internalFormat,
    bool is2D
)
{
    double requiredMemory = textureSize;
    double memoryPerPixel = 
        vir::TextureBuffer::internalFormatToBytes.at(internalFormat);
    requiredMemory *= memoryPerPixel;
    // Show a warning for good measure when creating beefier textures,
    // threshold arbitrarily set at 64 MiB of VRAM
    if (requiredMemory >= 67108864)
    {
        // Mip maps occupy a theoretical maximum of an additional 
        // 1/8 + 1/64 + 1/512 + 1/4096 + 1/... = 1/7 of the memory
        // occupied by the base level of a 3D texture, while for
        // 2D textures this is 1/4 + 1/16 + 1/32 + ... = 1/3 of the
        // memory occupied by the base level
        double mipmapsMemory = 
            std::floor(requiredMemory/memoryPerPixel/(is2D?3:7))*memoryPerPixel;
        auto uom1 = Helpers::autoRescaleMemoryValue(requiredMemory);
        auto uom2 = Helpers::autoRescaleMemoryValue(mipmapsMemory);
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f,1.f,0.f,1.f});
        ImGui::Text(
R"(This texture will occupy at least 
%.1f %s of free VRAM, and up to 
an additional %.1f %s for mipmaps. 
Please, make sure your system has 
at least the reported amount of 
free VRAM to avoid program and/or 
system crashes)",
            requiredMemory, 
            uom1,
            mipmapsMemory, 
            uom2
        );
        ImGui::PopStyleColor();
    }
}

//----------------------------------------------------------------------------//

bool Resource::createOrEditAnimationButtonGui
(
    Resource*& resource,
    const std::vector<Resource*>& resources,
    const ImVec2 size
)
{
    bool valid = false;
    static unsigned int width(0);
    static unsigned int height(0);
    static std::vector<Texture2DResource*> frames(0);
    static std::vector<std::string> orderedFrameNames(0);
    if 
    (
        ImGui::Button
        (
            resource == nullptr ? 
            "Create animation" : 
            "Replace",
            size
        )
    )
    {
        ImGui::OpenPopup("##createOrEditAnimationPopup");
        width = 0;
        height = 0;
        if (resource != nullptr)
        {
            auto animation = (AnimatedTexture2DResource*)resource;
            frames = std::vector<Texture2DResource*>
            (
                animation->unmanagedFrames_
            );
            int n(animation->unmanagedFrames_.size());
            orderedFrameNames.resize(n);
            for (int i=0; i<n; i++)
                orderedFrameNames[i] = std::to_string(i+1) + " - " +
                    animation->unmanagedFrames_[i]->name();
        }
    }
    if (ImGui::BeginPopup("##createOrEditAnimationPopup"))
    {
        ImGui::SeparatorText("Animation frames");
        int nFrames = frames.size();
        if (width == 0 && height == 0 && nFrames == 1)
        {
            width = frames[0]->width();
            height = frames[0]->height();
        }
        static bool reordered(false);
        int iFrameToBeDeleted = -1;
        ImGui::BeginChild
        (
            "##framesChild", 
            ImVec2
            (
                ImGui::GetContentRegionAvail().x, 
                std::min
                (
                    (float)std::max(nFrames, 1), 
                    15.f
                )*
                ImGui::GetTextLineHeightWithSpacing()
            ), 
            false
        );
        for (int i=0; i<nFrames; i++)
        {
            auto* frame = frames[i];
            auto name = orderedFrameNames[i];
            ImGui::PushID(i); 
            if (ImGui::SmallButton(ICON_FA_TRASH))
                iFrameToBeDeleted=i;
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Selectable
            (
                name.c_str(), 
                false, 
                ImGuiSelectableFlags_DontClosePopups
            );
            if 
            (
                ImGui::IsItemActive() && 
                !ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)
            )
            {
                int j = i + (ImGui::GetMouseDragDelta(0).y<0.f?-1:1);
                if (j >= 0 && j < nFrames)
                {
                    reordered = true;
                    orderedFrameNames[i] = orderedFrameNames[j];
                    orderedFrameNames[j] = name;
                    frames[i] = frames[j];
                    frames[j] = frame;
                    ImGui::ResetMouseDragDelta();
                }
            }
        }
        // As previously said, only re-order if the mouse button is no 
        // longer pressed
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && reordered)
        {
            for (int i=0; i<nFrames; i++)
                orderedFrameNames[i] = 
                    std::to_string(i+1)+" - "+
                    std::string(frames[i]->name());
            reordered = false;
        }
        ImGui::EndChild();
        if (iFrameToBeDeleted != -1)
        {
            frames.erase(frames.begin()+iFrameToBeDeleted);
            orderedFrameNames.erase
            (
                orderedFrameNames.begin()+iFrameToBeDeleted
            );
        }
        if
        (
            ImGui::BeginCombo
            (
                "##addAnimationFrameCombo",
                "Select frame to add"
            )
        )
        {
            for (auto* r : resources)
            {
                if 
                (
                    r->type_ != Resource::Type::Texture2D ||
                    (
                        width*height > 0 && 
                        (
                            r->width() != width ||
                            r->height() != height
                        )
                    )
                )
                    continue;
                if (ImGui::Selectable(r->name().c_str()))
                {
                    frames.emplace_back((Texture2DResource*)r);
                    orderedFrameNames.emplace_back
                    (
                        std::to_string(frames.size()) +
                        " - " + r->name()
                    );
                }
            }
            ImGui::EndCombo();
        }
        if (nFrames == 0)
            ImGui::BeginDisabled();
        if 
        (
            ImGui::Button
            (
                resource == nullptr ? "Create animation" : "Edit animation", 
                ImVec2(-1,0)
            )
        )
        {
            auto newResource = Resource::create(frames);
            if (newResource != nullptr)
            {
                if (resource != nullptr)
                    delete resource;
                resource = newResource;
                valid = true;
            }
            else
                valid = false;
            frames.clear();
            orderedFrameNames.clear();
        }
        if (nFrames == 0)
            ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    return valid;
}

//----------------------------------------------------------------------------//

bool Resource::createOrEditCubemapButtonGui
(
    Resource*& resource,
    const std::vector<Resource*>& resources,
    const ImVec2 size
)
{
    bool validSelection = false;
    static const Texture2DResource* selectedTextureResources[6]
    {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    };
    static unsigned int width = 0u;
    static unsigned int height = 0u;
    if 
    (
        ImGui::Button
        (
            resource == nullptr ? "Create cubemap" : "Replace", 
            size
        )
    )
    {
        ImGui::OpenPopup("##createOrReplaceCubeMapPopup");
        width = 0;
        height = 0;
        if (resource == nullptr)
        {
            for (int i=0;i<6;i++)
                selectedTextureResources[i] = nullptr;
        }
        else
        {
            auto cubemap = (CubemapResource*)resource;
            for (int i=0;i<6;i++)
                selectedTextureResources[i] = 
                    cubemap->unmanagedFaces_[i];
        }
    }   
    if (ImGui::BeginPopup("##createOrReplaceCubeMapPopup"))
    {
        static std::string labels[6] = 
        {
            "X+  ", "X-  ", "Y+  ", "Y-  ", "Z+  ", "Z-  "
        };
        int textureResourcei(0);
        int nSelectedTextureResources(0);
        for (int i=0; i<6; i++)
        {
            if (selectedTextureResources[i] != nullptr)
            {
                nSelectedTextureResources++;
                textureResourcei = i;
            }
        }
        
        bool validFaces = true;
        for (int i=0; i<6; i++)
        {
            ImGui::Text(labels[i].c_str());
            ImGui::SameLine();
            std::string selectedTextureResourceName = 
                selectedTextureResources[i] != nullptr ?
                selectedTextureResources[i]->name() : "";
            ImGui::PushItemWidth(-1);
            std::string comboi = 
                "##cubeMapFaceResourceSelector"+std::to_string(i);
            if 
            (
                ImGui::BeginCombo
                (
                    comboi.c_str(), 
                    selectedTextureResourceName.c_str()
                )
            )
            {
                for(int j=0; j<(int)resources.size()+1 ;j++)
                {
                    if (j==0)
                    {
                        if (ImGui::Selectable("None"))
                        {
                            selectedTextureResources[i] = nullptr;
                            if (nSelectedTextureResources == 1)
                            {
                                width=0;
                                height=0;
                            }
                        }
                        continue;
                    }
                    else if (resources[j-1]->type_ != Resource::Type::Texture2D)
                        continue;
                    auto r = (const Texture2DResource*)resources[j-1];
                    if 
                    (
                        !vir::CubeMapBuffer::validFace(r->native_)
                    )
                        continue;
                    if 
                    (
                        width != 0 && 
                        height != 0 && 
                        (
                            r->width() != width || 
                            r->height() != height
                        ) &&
                        !(
                            nSelectedTextureResources == 1 && 
                            i == textureResourcei
                        )
                    )
                        continue;
                    if (ImGui::Selectable(r->name().c_str()))
                    {
                        selectedTextureResources[i] = r;
                        width = r->width();
                        height = r->height();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            if (selectedTextureResources[i] == nullptr)
                validFaces = false;
        }
        float buttonSize(ImGui::GetFontSize()*15.0);
        if (!validFaces)
        {
            ImGui::BeginDisabled();
            ImGui::Button("Create cubemap", ImVec2(buttonSize, 0));
            ImGui::EndDisabled();
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text(
R"(To generate a cube map, select a texture from the 
loaded Texture2D resources for each of the 6 faces 
of the cubemap. Said resources need to: 
    1) have a square aspect ratio; 
    2) have the same resolution; 
    3) have a resoluton which is a power of 2. 
The available textures are automatically filtered 
among those loaded in the resource manager.)");
                ImGui::EndTooltip();
            }
        }
        else if (ImGui::Button("Create cubemap", ImVec2(buttonSize, 0)))
        {
            auto newResource = Resource::create(selectedTextureResources);
            if (newResource != nullptr)
            {
                if (resource != nullptr)
                    delete resource;
                resource = newResource;
                validSelection = true;
            }
        }
        ImGui::EndPopup();
    }
    return validSelection;
}

//----------------------------------------------------------------------------//

bool Resource::exportTexture2DOrAnimationButtonGui
(
    const Resource* resource,
    const ImVec2 size
)
{
    if 
    (
        resource == nullptr || 
        (
            resource->type_ != Resource::Type::Texture2D &&
            resource->type_ != Resource::Type::AnimatedTexture2D
        )
    )
        return false;
    /*if (auto texture2D = dynamic_cast<const Texture2DResource*>(resource))
    {
        if (texture2D->rawDataSize_ == 0 || texture2D->rawData_ == 0)
            return false;
    }*/
    if (ImGui::Button("Export", size))
    {
        std::string originalFileExtension = 
            resource->type_ == Resource::Type::Texture2D ?
            ((const Texture2DResource*)resource)->originalFileExtension_ :
            ((const AnimatedTexture2DResource*)resource)->originalFileExtension_;
        if (originalFileExtension.size() == 0)
        {
            if (resource->type_ == Resource::Type::Texture2D)
                originalFileExtension = ".png";
            else
                originalFileExtension = ".gif";
        }
        originalFileExtension = "*"+originalFileExtension;
        Resource::fileDialog_.runSaveFileDialog
        (
            "Re-export resource",
            {originalFileExtension, originalFileExtension},
            resource->name()
        );
        Resource::resourceToBeExported_ = resource;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------//

void Resource::renderResourceActionsButtonGui
(
    Resource*& resource,
    bool& deleteResource,
    const std::vector<Resource*>& resources,
    const ImVec2 size
)
{
    if (resource->type_ != Resource::Type::Framebuffer)
    {
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1,0)))
            ImGui::OpenPopup("##textureManagerSettings");
        if (ImGui::BeginPopup("##textureManagerSettings"))
        {
            std::vector<std::string*> inUseByCubemapOrAnimation(0);
            if (resource->type_ == Resource::Type::Texture2D)
            {
                for (auto* r : resources)
                {
                    if (r->type_ == Resource::Type::Cubemap)
                    {
                        auto cubemap = (const CubemapResource*)r;
                        auto faces = cubemap->unmanagedFaces_;
                        for (int i=0; i<6; i++)
                        {
                            if (faces[i]->name() != resource->name())
                                continue;
                            inUseByCubemapOrAnimation.emplace_back
                            (
                                cubemap->namePtr_
                            );
                            break;
                        }
                    }
                    else if 
                    (
                        r->type_ == Resource::Type::AnimatedTexture2D
                    )
                    {
                        auto animation = 
                            (const AnimatedTexture2DResource*)r;
                        for (auto& frame : animation->unmanagedFrames_)
                        {
                            if (frame->name() != resource->name())
                                continue;
                            inUseByCubemapOrAnimation.emplace_back(r->namePtr_);
                            break;
                        }
                    }
                }
            }
            //------------------------------------------------------------------

            bool settingsOpened = false;
            if (ImGui::Button("Settings", size))
            {
                settingsOpened = true;
                ImGui::OpenPopup("##resourceSettings");
            }
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
                inUseByCubemapOrAnimation.size() > 0
            )
            {
                // Again, if inUseByCubemapOrAnimation.size() > 0 I am assured 
                // this resource is a Texture2D, not a Cubemap nor an Animation
                ImGui::BeginTooltip();
                ImGui::Text(
R"(These settings only affect this texture and do not 
affect any cubemaps or animations using this texture)");
                ImGui::EndTooltip();
            }
            if (ImGui::BeginPopup("##resourceSettings"))
            {
                if (resource->type_ == Resource::Type::AnimatedTexture2D)
                {
                    //----------------------------------------------------------
                    auto animation = 
                        (AnimatedTexture2DResource*)resource;
                    auto nativeAnimation = animation->native_;
                    unsigned int frameIndex = nativeAnimation->frameIndex();
                    unsigned int nFrames = nativeAnimation->nFrames();
                    ImGui::Text("Animation frame     ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Button
                    (   
                        std::to_string(frameIndex+1).c_str(),
                        ImVec2(size.x/3.25, 0)
                    );
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    ImGui::Text("/ %d", nFrames);
                    ImGui::Text("Animation timing    ");
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            animation->isAnimationBoundToGlobalTime_ ?
                            "Bound to iTime" :
                            "Manual control",
                            size
                        )
                    )
                    {
                        animation->isAnimationBoundToGlobalTime_ = 
                            !animation->isAnimationBoundToGlobalTime_;
                    }
                    if (animation->isAnimationBoundToGlobalTime_)
                        ImGui::BeginDisabled();
                    ImGui::Text("Animation controls  ");
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            ICON_FA_STEP_BACKWARD, 
                            ImVec2(size.x/3.25, 0)
                        )
                    )
                        nativeAnimation->previousFrame(); // Step backwards
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            animation->isAnimationPaused_ ? 
                            ICON_FA_PLAY : 
                            ICON_FA_PAUSE,
                            ImVec2(size.x/3.25, 0)
                        )
                    )
                        animation->isAnimationPaused_ = 
                            !animation->isAnimationPaused_;
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            ICON_FA_STEP_FORWARD, 
                            ImVec2(-1, 0)
                        )
                    )
                        nativeAnimation->nextFrame();
                    if (animation->isAnimationBoundToGlobalTime_)
                        ImGui::EndDisabled();
                    ImGui::Text("Animation FPS       ");
                    ImGui::SameLine();
                    float fps(nativeAnimation->fps());
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::DragFloat
                        (
                            "##animationFpsDragFloat",
                            &fps,
                            0.1f,
                            0.1f,
                            160.0f,
                            "%.1f"
                        )
                    )
                    {
                        fps = std::min(std::max(0.1f, fps), 1000.f);
                        nativeAnimation->setFps(fps);
                    }
                    ImGui::Text("Animation duration  ");
                    ImGui::SameLine();
                    float duration(nativeAnimation->duration());
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::DragFloat
                        (
                            "##animationDurationDragFloat",
                            &duration,
                            0.1f,
                            nFrames/1000.0f,
                            nFrames/.1f,
                            "%.3f"
                        )
                    )
                    {
                        fps = nFrames/duration;
                        nativeAnimation->setFps(fps);
                    }
                    ImGui::PopItemWidth();
                    ImGui::Separator();
                }
                std::string selectedWrapModeX = "";
                std::string selectedWrapModeY = "";
                std::string selectedMagFilterMode = "";
                std::string selectedMinFilterMode = "";
                selectedWrapModeX=vir::TextureBuffer::wrapModeToName.at
                (
                    resource->wrapMode(0)
                );
                selectedWrapModeY=vir::TextureBuffer::wrapModeToName.at
                (
                    resource->wrapMode(1)
                );
                selectedMagFilterMode = 
                    vir::TextureBuffer::filterModeToName.at
                    (
                        resource->magFilterMode()
                    );
                selectedMinFilterMode = 
                    vir::TextureBuffer::filterModeToName.at
                    (
                        resource->minFilterMode()
                    );
                if (resource->type_ != Resource::Type::Cubemap)
                {
                    ImGui::Text("Texture wrap mode H ");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::BeginCombo
                        (
                            "##bufferWrapModeXCombo",
                            selectedWrapModeX.c_str()
                        )
                    )
                    {
                        for (auto e:vir::TextureBuffer::wrapModeToName)
                            if (ImGui::Selectable(e.second.c_str()))
                                resource->setWrapMode(0, e.first);
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                    ImGui::Text("Texture wrap mode V ");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::BeginCombo
                        (
                            "##bufferWrapModeYCombo",
                            selectedWrapModeY.c_str()
                        )
                    )
                    {
                        for (auto e:vir::TextureBuffer::wrapModeToName)
                            if (ImGui::Selectable(e.second.c_str()))
                                resource->setWrapMode(1, e.first);
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                }
                ImGui::Text("Texture mag. filter ");
                ImGui::SameLine();
                ImGui::PushItemWidth(size.x);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##bufferMagModeCombo",
                        selectedMagFilterMode.c_str()
                    )
                )
                {
                    for (auto e : vir::TextureBuffer::filterModeToName)
                    {
                        // Mipmap filters are for min only,
                        // no effect on mag, hence they 
                        // are skipped here
                        if
                        (
                            e.first != 
                            vir::TextureBuffer::FilterMode::Nearest &&
                            e.first != 
                            vir::TextureBuffer::FilterMode::Linear
                        )
                            continue;
                        if (ImGui::Selectable(e.second.c_str()))
                            resource->setMagFilterMode(e.first);
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                ImGui::Text("Texture min. filter ");
                ImGui::SameLine();
                ImGui::PushItemWidth(size.x);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##bufferMinModeCombo",
                        selectedMinFilterMode.c_str()
                    )
                )
                {
                    for (auto e : vir::TextureBuffer::filterModeToName)
                    {
                        if 
                        (
                            resource->isInternalFormatUnsigned() &&
                            e.first != FilterMode::Linear && 
                            e.first != FilterMode::Nearest
                        )
                            continue;
                        if (ImGui::Selectable(e.second.c_str()))
                            resource->setMinFilterMode(e.first);
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                bool usesMipmap = 
                    resource->minFilterMode() != FilterMode::Nearest &&
                    resource->minFilterMode() != FilterMode::Linear;
                if (resource->type_ == Resource::Type::Texture2D)
                {
                    auto texture = (Texture2DResource*)resource;
                    if (usesMipmap)
                    {
                        ImGui::Text("Auto mipmap update  ");
                        ImGui::SameLine();
                        ImGui::Checkbox
                        (
                            "##autoMipmapUpdate", 
                            &(texture->autoUpdateMipmap)
                        );
                        if 
                        (
                            !texture->autoUpdateMipmap && 
                            ImGui::Button("Update mipmap", ImVec2(-1, 0))
                        )
                            texture->updateMipmap();
                    }
                    if 
                    (
                        texture->rawData_ == nullptr ||
                        texture->rawDataSize_ == 0
                    )
                    {
                        bool disabled = inUseByCubemapOrAnimation.size() > 0;
                        if (disabled)
                            ImGui::BeginDisabled();
                        createOrResizeOrReformatTexture2DGui
                        (
                            resource,
                            false,
                            settingsOpened
                        );
                        if (disabled)
                            ImGui::EndDisabled();
                    }
                }
                else if (resource->type_ == Resource::Type::Texture3D)
                {
                    auto texture = (Texture3DResource*)resource;
                    if (usesMipmap)
                    {
                        ImGui::Text("Auto mipmap update  ");
                        ImGui::SameLine();
                        ImGui::Checkbox
                        (
                            "##autoMipmapUpdate", 
                            &(texture->autoUpdateMipmap)
                        );
                        if 
                        (
                            !texture->autoUpdateMipmap && 
                            ImGui::Button("Update mipmap", ImVec2(-1, 0))
                        )
                            texture->updateMipmap();
                    }
                    createOrResizeOrReformatTexture3DGui
                    (
                        resource,
                        false,
                        settingsOpened
                    );
                }
                else if 
                (
                    resource->type_ == Resource::Type::AnimatedTexture2D &&
                    usesMipmap
                )
                {
                    ImGui::Text("Auto mipmap update  ");
                    ImGui::SameLine();
                    ImGui::Checkbox
                    (
                        "##autoMipmapUpdate", 
                        &(((AnimatedTexture2DResource*)resource)->autoUpdateMipmap)
                    );
                    // No manual mipmap update button for animation-type
                    // resources because it just does not make much sense (also,
                    // the way it's currently implemented in vir, it only 
                    // updates the current animation frame)
                }
                ImGui::EndPopup();
            }
            //------------------------------------------------------------------
            if (inUseByCubemapOrAnimation.size() == 0)
            {
                switch (resource->type_)
                {
                    case Resource::Type::Texture2D:
                        loadOrReplaceTexture2DOrAnimationButtonGui
                        (
                            resource,
                            size, 
                            false
                        );
                        break;
                    case Resource::Type::AnimatedTexture2D:
                        
                        ((AnimatedTexture2DResource*)resource)->
                        unmanagedFrames_.size() == 0 ? 
                        loadOrReplaceTexture2DOrAnimationButtonGui
                        (
                            resource,
                            size, 
                            true
                        ) :
                        createOrEditAnimationButtonGui
                        (
                            resource,
                            resources,
                            size
                        );
                        break;
                    case Resource::Type::Cubemap:
                        createOrEditCubemapButtonGui
                        (
                            resource,
                            resources,
                            size
                        );
                        break;
                    default:
                        break;
                }
            }
            else    // No way inUseByCubemapOrAnimation.size() != 0 if 
                    // resource->type_ != Resource::Type::Texture2D, so no 
                    // cubemaps or animations here
            {
                loadOrReplaceTexture2DOrAnimationButtonGui
                (
                    resource,
                    size,
                    false,
                    true
                );
                if 
                (
                    ImGui::IsItemHovered
                    (
                        ImGuiHoveredFlags_AllowWhenDisabled
                    ) && ImGui::BeginTooltip()
                )
                {
                    std::string hoverText = 
"This texture is in use by the following resources:\n";
                    for (int i=0; i<(int)inUseByCubemapOrAnimation.size(); i++)
                        hoverText += 
                            "  "+std::to_string(i+1)+") "+
                            *inUseByCubemapOrAnimation[i]+"\n";
                    hoverText += 
"To replace this texture, first delete the resources which use it";
                    ImGui::Text(hoverText.c_str());
                    ImGui::EndTooltip();
                }
            }
            //------------------------------------------------------------------
            if 
            (
                resource->type_ == Resource::Type::Texture2D ||
                resource->type_ == Resource::Type::AnimatedTexture2D
            )
                exportTexture2DOrAnimationButtonGui(resource,size);
            //------------------------------------------------------------------
            if (inUseByCubemapOrAnimation.size() == 0)
            {
                if (ImGui::Button("Delete", size))
                    deleteResource = true;
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button("Delete", size);
                ImGui::EndDisabled();
                if 
                (
                    ImGui::IsItemHovered
                    (
                        ImGuiHoveredFlags_AllowWhenDisabled
                    ) && ImGui::BeginTooltip()
                )
                {
                    std::string hoverText = 
"This texture is in use by the following resources:\n";
                    for (int i=0; i<(int)inUseByCubemapOrAnimation.size(); i++)
                        hoverText += 
                            "  "+std::to_string(i+1)+") "+
                            *inUseByCubemapOrAnimation[i]+"\n";
                    hoverText += 
"To delete this texture, first delete the resources which use it";
                    ImGui::Text(hoverText.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndPopup();
        }
    }
    else
    {
        if (ImGui::Button(ICON_FA_COG, ImVec2(-1,0)))
            ImGui::OpenPopup("##layerFramebufferSettings");
        if (ImGui::BeginPopup("##layerFramebufferSettings"))
        {
            ((LayerResource*)resource)->layer_->renderFramebufferPropertiesGui();
            ImGui::EndPopup();
        }
    }
}

}