/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "resources/resource.h"
#include "objectio/objectio.h"

#include "thirdparty/stb/stb_image.h"

#include <ctime>
#include <iostream>

namespace ShaderThing
{

// Static members ------------------------------------------------------------//

std::unordered_map<Resource::Type, std::string> Resource::typeToName = 
{
    {Resource::Type::Uninitialized, "Uninitialized"},
    {Resource::Type::Texture2D, "Texture-2D"},
    {Resource::Type::AnimatedTexture2D, "Animation-2D"},
    {Resource::Type::FramebufferColorAttachment, "Layer"},
    {Resource::Type::Cubemap, "Cubemap"}
};

std::unordered_map<std::string, Resource::Type> Resource::nameToType = 
{
    {"Uninitialized", Resource::Type::Uninitialized},
    {"Texture-2D", Resource::Type::Texture2D},
    {"Animation-2D", Resource::Type::AnimatedTexture2D},
    {"Layer", Resource::Type::FramebufferColorAttachment},
    {"Cubemap", Resource::Type::Cubemap}
};

bool Resource::validCubemapFace(Resource* face)
{
    int width(face->width());
    int height(face->height());
    if (width != height)
        return false;
    auto isPowerOfTwo = [](int x)->bool{return(x!=0)&&((x&(x-1))==0);};
    if (!isPowerOfTwo(width) || !isPowerOfTwo(height))
        return false;
    return true;
}

bool Resource::validCubemapFaces(Resource* faces[6])
{
    int width, height;
    auto isPowerOfTwo = [](int x)->bool{return(x!=0)&&((x&(x-1))==0);};
    for (int i=0; i<6; i++)
    {
        if (faces[i]->rawDataSize() == 0)
            return false;
        if (i==0)
        {
            width = faces[i]->width();
            height = faces[i]->height();
            if (width != height)
                return false;
        }
        else if (faces[i]->width() != width || faces[i]->height() != height)
            return false;
        if (!isPowerOfTwo(width) || !isPowerOfTwo(height))
            return false;
    }
    return true;
}

// Macros --------------------------------------------------------------------//

#define NATIVE_RESOURCE(nativeType) static_cast<nativeType*>(nativeResource_)

#define SET_NATIVE_RESOURCE(resourceType)                                   \
    if (nativeResource == nullptr) return false;                            \
    if(valid())reset();                                                     \
    type_ = resourceType;                                                   \
    nativeResource_ = (void*)nativeResource;

#define DELETE_NATIVE_RESOUCE(nativeType) {                                 \
    auto resource = NATIVE_RESOURCE(nativeType);                            \
    if (lastBoundUnit_ != -1) resource->unbind(lastBoundUnit_);             \
    delete resource;                                                        \
    break;}

#define CALL_NATIVE_RESOURCE_FUNC_HOMO(func) {                              \
    switch (type_) {                                                        \
        case Type::FramebufferColorAttachment :                             \
            return (*NATIVE_RESOURCE(vir::Framebuffer*))->func;             \
        case Type::AnimatedTexture2D :                                      \
            return NATIVE_RESOURCE(vir::AnimatedTextureBuffer2D)->func;     \
        case Type::Texture2D :                                              \
            return NATIVE_RESOURCE(vir::TextureBuffer2D)->func;             \
        case Type::Cubemap :                                                \
            return NATIVE_RESOURCE(vir::CubeMapBuffer)->func;               \
    }}

#define CALL_NATIVE_RESOURCE_FUNC_HETERO2(func0, func1) {                   \
    switch (type_) {                                                        \
        case Type::FramebufferColorAttachment :                             \
            return (*NATIVE_RESOURCE(vir::Framebuffer*))->func0;            \
        case Type::Texture2D :                                              \
            return NATIVE_RESOURCE(vir::TextureBuffer2D)->func1;            \
        case Type::AnimatedTexture2D :                                      \
            return NATIVE_RESOURCE(vir::AnimatedTextureBuffer2D)->func1;    \
        case Type::Cubemap :                                                \
            return NATIVE_RESOURCE(vir::CubeMapBuffer)->func1;              \
    }}

#define CALL_NATIVE_RESOURCE_FUNC_HETERO3(func0, func1, func2) {            \
    switch (type_) {                                                        \
        case Type::FramebufferColorAttachment :                             \
            return (*NATIVE_RESOURCE(vir::Framebuffer*))->func0;            \
        case Type::Texture2D :                                              \
            return NATIVE_RESOURCE(vir::TextureBuffer2D)->func1;            \
        case Type::AnimatedTexture2D :                                      \
            return NATIVE_RESOURCE(vir::AnimatedTextureBuffer2D)->func2;    \
        case Type::Cubemap :                                                \
            return NATIVE_RESOURCE(vir::CubeMapBuffer)->func1;              \
    }}

// Public functions ----------------------------------------------------------//

Resource::Resource() :
type_(Resource::Type::Uninitialized),
nativeResource_(nullptr),
referencedResources_(0),
namePtr_(new std::string()),
originalFileExtension_(""),
rawDataSize_(0),
rawData_(nullptr),
lastBoundUnit_(-1),
animationData_(nullptr)
{}

Resource::~Resource()
{
    reset();
    if 
    (
        namePtr_ != nullptr 
        && type_!=Resource::Type::FramebufferColorAttachment
    )
        delete namePtr_;
    namePtr_ = nullptr;
}

void Resource::reset()
{
    if (rawData_ != nullptr) delete[] rawData_;
    rawData_ = nullptr;
    if (!valid()) return;
    switch (type_)
    {
        case Type::FramebufferColorAttachment : break;
        case Type::Texture2D : DELETE_NATIVE_RESOUCE(vir::TextureBuffer2D)
        case Type::AnimatedTexture2D : DELETE_NATIVE_RESOUCE(
            vir::AnimatedTextureBuffer2D)
        case Type::Cubemap : DELETE_NATIVE_RESOUCE(vir::CubeMapBuffer)
    }
    nativeResource_ = nullptr;
    referencedResources_.resize(0);
    if (animationData_ != nullptr)
        delete animationData_;
    animationData_ = nullptr;
}

void Resource::update(float globalTime, float timeStep)
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    animationData_->internalTime = 
        animationData_->isInternalTimeBoundToGlobalTime ?
        globalTime :
        animationData_->internalTime + 
            (!animationData_->isInternalTimePaused)*timeStep;
    animation->setFrameIndexFromTime(animationData_->internalTime);
}

void Resource::bind(int unit)
{
    if (!valid()) return;
    lastBoundUnit_ = unit;
    CALL_NATIVE_RESOURCE_FUNC_HETERO2(bindColorBuffer(unit), bind(unit))
}

void Resource::unbind(int unit)
{
    if (!valid()) return;
    if (unit == -1)
    {
        if (lastBoundUnit_ == -1) return;
        else unit = lastBoundUnit_;
    }
    CALL_NATIVE_RESOURCE_FUNC_HETERO2(unbind(), unbind(unit))
}

int Resource::id(bool contentId) const
{
    if (!valid()) return -1;
    if (!contentId)
        CALL_NATIVE_RESOURCE_FUNC_HETERO2(colorBufferId(), id())
    else
        CALL_NATIVE_RESOURCE_FUNC_HETERO3(colorBufferId(), id(), 
            currentFrameId())
    return -1;
}

int Resource::width() const
{
    if (valid()) 
        CALL_NATIVE_RESOURCE_FUNC_HOMO(width())
    return 0;
}

int Resource::height() const
{
    if (valid())
        CALL_NATIVE_RESOURCE_FUNC_HOMO(height())
    return 0;
}

vir::TextureBuffer::WrapMode Resource::wrapMode(int i)
{
    if (valid()) 
        CALL_NATIVE_RESOURCE_FUNC_HETERO2(colorBufferWrapMode(i), 
            wrapMode(i))
    return vir::TextureBuffer::WrapMode::ClampToBorder;
}

vir::TextureBuffer::FilterMode Resource::magFilterMode()
{
    if (valid()) 
        CALL_NATIVE_RESOURCE_FUNC_HETERO2(colorBufferMagFilterMode(), 
            magFilterMode())
    return vir::TextureBuffer::FilterMode::Nearest;
}

vir::TextureBuffer::FilterMode Resource::minFilterMode()
{
    if (valid())
        CALL_NATIVE_RESOURCE_FUNC_HETERO2(colorBufferMinFilterMode(), 
            minFilterMode())
    return vir::TextureBuffer::FilterMode::Nearest;
}

void Resource::toggleAnimationPaused()
{
    if (type_ != Type::AnimatedTexture2D || animationData_ == nullptr)
        return;
    animationData_->isInternalTimePaused = 
        !animationData_->isInternalTimePaused;
}

void Resource::toggleAnimationBoundToGlobalTime()
{
     if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    animationData_->isInternalTimeBoundToGlobalTime = 
        !animationData_->isInternalTimeBoundToGlobalTime;
}

void Resource::stepAnimationForwards()
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    animation->nextFrame();
    animationData_->internalTime += animation->frameDuration();
}

void Resource::stepAnimationBackwards()
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    animation->previousFrame();
    animationData_->internalTime -= animation->frameDuration();
}

float Resource::animationDuration() const
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return 0.f;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    return animation->frameDuration()*animation->nFrames();
}

unsigned int Resource::animationFrameIndex() const
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return 0;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    return animation->currentFrameIndex();
}

unsigned int Resource::nAnimationFrames() const
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return 0;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    return animation->nFrames();
}

template<>
bool Resource::set(vir::Framebuffer** nativeResource)
{
    SET_NATIVE_RESOURCE(Type::FramebufferColorAttachment)
    return true;
}

template<>
bool Resource::set(vir::TextureBuffer2D* nativeResource)
{
    SET_NATIVE_RESOURCE(Type::Texture2D)
    return true;
}

template<>
bool Resource::set(vir::AnimatedTextureBuffer2D* nativeResource)
{
    SET_NATIVE_RESOURCE(Type::AnimatedTexture2D)
    animationData_ = new AnimationData{};
    return true;
}

template<>
bool Resource::set(vir::CubeMapBuffer* nativeResource)
{
    SET_NATIVE_RESOURCE(Type::Cubemap)
    return true;
}

// This is specifically for creating a Texture2D or AnimatedTexture2D from an
// image file (.jpg/.png/.bmp for the former and .gif for the latter)
template<>
bool Resource::set(std::string filepath)
{
    // Load image data
    std::ifstream rawDataStream
    (
        filepath, std::ios::binary | std::ios::in
    );
    rawDataStream.seekg(0, std::ios::end);
    size_t rawDataSize = rawDataStream.tellg();
    rawDataStream.seekg(0, std::ios::beg);
    unsigned char* rawData = new unsigned char[rawDataSize];
    rawDataStream.read((char*)rawData, rawDataSize);
    rawDataStream.close();

    // Read file extension to check if GIF
    std::string fileExtension = "";
    bool foundDot(false);
    for (int i=filepath.size()-1; i>=0; i--)
    {
        char& c(filepath[i]);
        if (!foundDot)
        {
            foundDot = (c == '.');
            fileExtension = c + fileExtension;
        }
        else break;
    }

    if (fileExtension != ".gif") // If not gif, regular Texture2D
    {
        try
        {
            set
            (
                vir::TextureBuffer2D::create
                (
                    filepath, 
                    vir::TextureBuffer::InternalFormat::RGBA_UNI_8
                )
            );
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            delete[] rawData;
            return false;
        }
    }
    else // If gif, AnimatedTexture2D
    {
        try
        {
            set
            (
                vir::AnimatedTextureBuffer2D::create
                (
                    filepath, 
                    vir::TextureBuffer::InternalFormat::RGBA_UNI_8
                )
            );
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            delete[] rawData;
            return false;
        }
    }

    // Set original file extension and raw data if all went well
    originalFileExtension_ = fileExtension;
    setRawData(rawData, rawDataSize);

    return true;
}

// This is specifically for generating and setting AnimatedTexture2Ds from a
// list of existing resources
template<>
bool Resource::set(std::vector<Resource*>* resources)
{
    std::vector<vir::TextureBuffer2D*> frames(0);
    referencedResources_.resize(0);
    for(auto* r : *resources)
    {
        if (!r->valid() || r->type() != Type::Texture2D)
            continue;
        frames.emplace_back((vir::TextureBuffer2D*)r->nativeResource());
        referencedResources_.emplace_back(r);
    }
    auto animation = vir::AnimatedTextureBuffer2D::create
    (
        frames,
        false
    );
    return set(animation);
}

// This is specifically for generating and setting cubemaps
template<>
bool Resource::set(Resource* faces[6])
{
    // Check validity of provided faces
    if (!Resource::validCubemapFaces(faces)) return false;

    // Read face data
    const unsigned char* faceData[6];
    stbi_set_flip_vertically_on_load(false);
    int width, height, nChannels;
    for (int i=0; i<6; i++)
    {
        const unsigned char* rawData = faces[i]->rawData();
        int rawDataSize = faces[i]->rawDataSize();
        faceData[i] = stbi_load_from_memory
        (
            rawData, 
            rawDataSize, 
            &width, 
            &height, 
            &nChannels,
            4   // Force all textures to be treated as 4-component
        );
    }
    // Build cubemap
    try
    {
        set
        (
            vir::CubeMapBuffer::create
            (
                faceData, 
                width, 
                height, 
                vir::TextureBuffer::InternalFormat::RGBA_UNI_8
            )
        );
        for (int i=0; i<6; i++) // Free buffer memory used for loading
            stbi_image_free((void*)faceData[i]);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        for (int i=0; i<6; i++)
            stbi_image_free((void*)faceData[i]);
        return false;
    }

    // Finalize and store refs to faces
    referencedResources_.resize(0);
    for (int i=0; i<6; i++)
        referencedResources_.emplace_back(faces[i]);
    return true;
}

// Set Texture2D or AnimatedTexture2D from raw file data and size
bool Resource::set(const unsigned char* rawData, unsigned int size, bool gif)
{
    if (gif)
    {
        int w, h, nf, nc;
        int* fd;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load_gif_from_memory
        (
            rawData, 
            size, 
            &fd,
            &w, 
            &h, 
            &nf,
            &nc,
            4   // Force all textures to be treated as 4-component
        );
        nc = 4; // Force all textures to be treated as 4-component
        auto animation = vir::AnimatedTextureBuffer2D::create
        (
            data, 
            w, 
            h, 
            nf,
            vir::TextureBuffer::defaultInternalFormat(nc)
        );
        float dt = 0.f;
        for (int i=0;i<nf;i++)
            dt += fd[i];
        animation->setFrameDuration(dt/nf);
        stbi_image_free(fd);
        stbi_image_free(data);
        return this->set(animation);
    }
    else
    {
        int w, h, nc;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load_from_memory
        (
            rawData, 
            size, 
            &w, 
            &h, 
            &nc,
            4   // Force all textures to be treated as 4-component
        );
        nc = 4; // Force all textures to be treated as 4-component
        auto texture = vir::TextureBuffer2D::create
        (
            data, 
            w,
            h,
            vir::TextureBuffer::defaultInternalFormat(nc)
        ); 
        stbi_image_free(data);
        return this->set(texture);
    }
}

void Resource::setNamePtr(std::string* namePtr)
{
    if (namePtr_ != nullptr) delete namePtr_;
    namePtr_ = namePtr;
}

void Resource::setRawData(unsigned char* rawData, int rawDataSize)
{
    if (rawData_ != nullptr) delete[] rawData_;
    rawData_ = rawData;
    rawDataSize_ = rawDataSize;
}

void Resource::setWrapMode(int index, vir::TextureBuffer::WrapMode mode)
{
    if (!valid()) return;
    CALL_NATIVE_RESOURCE_FUNC_HETERO2
    (
        setColorBufferWrapMode(index, mode), 
        setWrapMode(index, mode)
    )
}

void Resource::setMagFilterMode(vir::TextureBuffer::FilterMode mode)
{
    if (!valid()) return;
    CALL_NATIVE_RESOURCE_FUNC_HETERO2
    (
        setColorBufferMagFilterMode(mode), 
        setMagFilterMode(mode)
    )
}

void Resource::setMinFilterMode(vir::TextureBuffer::FilterMode mode)
{
    if (!valid()) return;
    CALL_NATIVE_RESOURCE_FUNC_HETERO2
    (
        setColorBufferMinFilterMode(mode), 
        setMinFilterMode(mode)
    )
}

bool Resource::isAnimationPaused() const
{
     if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return false;
    return animationData_->isInternalTimePaused;
}

bool Resource::isAnimationBoundToGlobalTime() const
{
     if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return false;
    return animationData_->isInternalTimeBoundToGlobalTime;
}

float Resource::animationFps() const
{
     if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return 0.f;
    return 
        1.0/((vir::AnimatedTextureBuffer2D*)nativeResource_)->frameDuration();
}

void Resource::setAnimationFps(float fps)
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    auto frameIndex0 = animation->currentFrameIndex();
    animation->setFrameDuration(1.0/fps);
    setAnimationFrameIndex(frameIndex0);
}

void Resource::setAnimationDuration(float t)
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    auto frameIndex0 = animation->currentFrameIndex();
    animation->setFrameDuration(t/animation->nFrames());
    setAnimationFrameIndex(frameIndex0);
}

void Resource::setAnimationFrameIndex(unsigned int frameIndex)
{
    if (!valid() || type_!=Type::AnimatedTexture2D || animationData_==nullptr)
        return;
    auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
    animation->setFrameIndex(frameIndex);
    animationData_->internalTime = frameIndex*animation->frameDuration();
}

//----------------------------------------------------------------------------//

Resource::Resource
(
    const ObjectIO& reader, 
    const std::vector<Resource*>& resources
) :
type_(Resource::Type::Uninitialized),
nativeResource_(nullptr),
referencedResources_(0),
namePtr_(new std::string()),
originalFileExtension_(""),
rawDataSize_(0),
rawData_(nullptr),
lastBoundUnit_(-1),
animationData_(nullptr)
{
    type_ = nameToType.at(reader.read<std::string>("type"));
    auto readerName = std::string(reader.name());
    setNamePtr(new std::string(readerName));
    switch(type_)
    {
        case Type::Texture2D :
        {
            unsigned int rawDataSize;
            auto rawData = (unsigned char*)reader.read("data", true, 
                &rawDataSize);
            set(rawData, rawDataSize);
            setRawData(rawData, rawDataSize);
            setOriginalFileExtension(reader.read<std::string>(
                "originalFileExtension"));
            auto wrapModes = reader.read<glm::ivec2>("wrapModes");
            setWrapMode(0, (vir::TextureBuffer::WrapMode)wrapModes.x);
            setWrapMode(1, (vir::TextureBuffer::WrapMode)wrapModes.y);
            break;
        }
        case Type::AnimatedTexture2D :
        {
            if (reader.hasMember("data")) // it is a .gif
            {
                unsigned int rawDataSize;
                auto rawData = (unsigned char*)reader.read("data", true,
                    &rawDataSize);
                set(rawData, rawDataSize, true);
                setRawData(rawData, rawDataSize);
                setOriginalFileExtension(reader.read<std::string>(
                    "originalFileExtension"));
            }
            else // it is constructed from other resources
            {
                auto frameNames = reader.read<std::vector<std::string>>(
                    "frames");
                std::vector<Resource*> referencedResources(frameNames.size());
                int i = 0;
                for (auto& frameName : frameNames)
                {
                    for (auto r : resources)
                        if 
                        (
                            r->name() == frameName && 
                            r->type() == Resource::Type::Texture2D
                        )
                            referencedResources[i] = r;
                    ++i;
                }
                set(&referencedResources);
            }
            auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
            auto wrapModes = reader.read<glm::ivec2>("wrapModes");
            setWrapMode(0, (vir::TextureBuffer::WrapMode)wrapModes.x);
            setWrapMode(1, (vir::TextureBuffer::WrapMode)wrapModes.y);
            setAnimationFps(reader.read<float>("animationFps"));
            setAnimationFrameIndex
            (
                reader.read<uint32_t>("animationFrameIndex")
            );
            if (reader.read<bool>("animationPaused"))
                toggleAnimationPaused();
            if (reader.read<bool>("animationBoundToGlobalTime"))
                toggleAnimationBoundToGlobalTime();
            break;
        }
        case Type::Cubemap :
        {
            auto faceNames = reader.read<std::vector<std::string>>("faces");
            Resource* faces[6];
            int facei = 0;
            for (auto& faceName : faceNames)
            {
                for (auto r : resources)
                    if 
                    (
                        r->name() == faceName && 
                        r->type() == Resource::Type::Texture2D
                    )
                        faces[facei] = r;
                ++facei;
            }
            set(faces);
            break;
        }
    }
    setMagFilterMode
    (
        (vir::TextureBuffer::FilterMode)reader.read<int>
        (
            "magnificationFilterMode"
        )
    );
    setMinFilterMode
    (
        (vir::TextureBuffer::FilterMode)reader.read<int>
        (
            "minimizationFilterMode"
        )
    );
}

void Resource::saveState(ObjectIO& writer)
{
    if
        (
            type_ == Resource::Type::Uninitialized ||
            type_ == Resource::Type::FramebufferColorAttachment
        )
            return;

        writer.writeObjectStart(namePtr_->c_str());
        writer.write("type", Resource::typeToName[type_].c_str());
        writer.write("magnificationFilterMode", (int)magFilterMode());
        writer.write("minimizationFilterMode", (int)minFilterMode());
        switch (type_)
        {
        case Resource::Type::AnimatedTexture2D :
        {
            writer.write("wrapModes", 
                glm::ivec2((int)wrapMode(0), (int)wrapMode(1)));
            writer.write("animationFps", animationFps());
            writer.write("animationFrameIndex", animationFrameIndex());
            writer.write("animationPaused", isAnimationPaused());
            writer.write("animationBoundToGlobalTime", 
                isAnimationBoundToGlobalTime());
            if // it is a .gif
            (
                originalFileExtension_.size() > 0 && 
                rawData_ != nullptr &&
                rawDataSize_ > 0
            )
            {
                writer.write("originalFileExtension", 
                    originalFileExtension_.c_str());
                writer.write("data", (const char*)rawData_, rawDataSize_, true);
            }
            else // if it is an animation constructed from other resources
            {
                auto animation = (vir::AnimatedTextureBuffer2D*)nativeResource_;
                std::vector<std::string> frameNames(animation->nFrames());
                for (int i=0; i<frameNames.size(); i++)
                    frameNames[i] = referencedResources_[i]->name();
                writer.write("frames", frameNames);
            }
            break;
        }
        case Resource::Type::Texture2D :
        {
            writer.write("wrapModes", 
                glm::ivec2((int)wrapMode(0), (int)wrapMode(1)));
            writer.write("originalFileExtension", 
                originalFileExtension_.c_str());
            writer.write("data",(const char*)rawData_,rawDataSize_,true);
            break;
        }
        case Resource::Type::Cubemap :
        {
            std::vector<std::string> faceNames(6);
            for (int i=0; i<6; i++)
                faceNames[i] = referencedResources_[i]->name();
            writer.write("faces", faceNames);
            break;
        }
        }
        writer.writeObjectEnd();
}

}