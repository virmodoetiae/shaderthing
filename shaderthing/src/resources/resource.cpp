#include "resources/resource.h"

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
    {Resource::Type::FramebufferColorAttachment, "Buffer"},
    {Resource::Type::Cubemap, "Cubemap"}
};

std::unordered_map<std::string, Resource::Type> Resource::nameToType = 
{
    {"Uninitialized", Resource::Type::Uninitialized},
    {"Texture-2D", Resource::Type::Texture2D},
    {"Buffer", Resource::Type::FramebufferColorAttachment},
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
        case Type::Texture2D :                                              \
            return NATIVE_RESOURCE(vir::TextureBuffer2D)->func;             \
        case Type::Cubemap :                                                \
            return NATIVE_RESOURCE(vir::CubeMapBuffer)->func;               \
    }}

#define CALL_NATIVE_RESOURCE_FUNC_HETERO(func0, func1) {                    \
    switch (type_) {                                                        \
        case Type::FramebufferColorAttachment :                             \
            return (*NATIVE_RESOURCE(vir::Framebuffer*))->func0;            \
        case Type::Texture2D :                                              \
            return NATIVE_RESOURCE(vir::TextureBuffer2D)->func1;            \
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
lastBoundUnit_(-1)
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
        case Type::Cubemap : DELETE_NATIVE_RESOUCE(vir::CubeMapBuffer)
    }
    nativeResource_ = nullptr;
    referencedResources_.resize(0);
}

void Resource::bind(int unit)
{
    if (!valid()) return;
    lastBoundUnit_ = unit;
    CALL_NATIVE_RESOURCE_FUNC_HETERO(bindColorBuffer(unit), bind(unit))
}

void Resource::unbind(int unit)
{
    if (!valid()) return;
    if (unit == -1)
    {
        if (lastBoundUnit_ == -1) return;
        else unit = lastBoundUnit_;
    }
    CALL_NATIVE_RESOURCE_FUNC_HETERO(unbind(), unbind(unit))
}

int Resource::id() const
{
    if (valid())
        CALL_NATIVE_RESOURCE_FUNC_HETERO(colorBufferId(), id())
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
        CALL_NATIVE_RESOURCE_FUNC_HETERO(colorBufferWrapMode(i), 
            wrapMode(i))
    return vir::TextureBuffer::WrapMode::ClampToBorder;
}

vir::TextureBuffer::FilterMode Resource::magFilterMode()
{
    if (valid()) 
        CALL_NATIVE_RESOURCE_FUNC_HETERO(colorBufferMagFilterMode(), 
            magFilterMode())
    return vir::TextureBuffer::FilterMode::Nearest;
}

vir::TextureBuffer::FilterMode Resource::minFilterMode()
{
    if (valid())
        CALL_NATIVE_RESOURCE_FUNC_HETERO(colorBufferMinFilterMode(), 
            minFilterMode())
    return vir::TextureBuffer::FilterMode::Nearest;
}

template<>
bool Resource::set(vir::TextureBuffer2D* nativeResource)
{
    SET_NATIVE_RESOURCE(Type::Texture2D)
    return true;
}

template<>
bool Resource::set(vir::Framebuffer** nativeResource)
{
    SET_NATIVE_RESOURCE(Type::FramebufferColorAttachment)
    return true;
}

template<>
bool Resource::set(vir::CubeMapBuffer* nativeResource)
{
    SET_NATIVE_RESOURCE(Type::Cubemap)
    return true;
}

// This is specifically for generating AND setting texture2Ds
template<>
bool Resource::set(std::string filepath)
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
        return false;
    }

    // Set original file extension
    originalFileExtension_ = "";
    bool foundDot(false);
    for (int i=filepath.size()-1; i>=0; i--)
    {
        char& c(filepath[i]);
        if (!foundDot)
        {
            foundDot = (c == '.');
            originalFileExtension_ = c+originalFileExtension_;
        }
        else break;
    }

    // Also set raw data
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
    setRawData(rawData, rawDataSize);

    return true;
}

// This is specifically for generating AND setting cubemaps
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
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }

    // Finalize and store refs to faces
    referencedResources_.resize(0);
    for (int i=0; i<6; i++)
        referencedResources_.emplace_back(faces[i]);
    return true;
}

// Set Texture2D from actual data and data size
bool Resource::set(const unsigned char* data, unsigned int size)
{
    int w,h,nc;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* loadedData = stbi_load_from_memory
    (
        data, 
        size, 
        &w, 
        &h, 
        &nc,
        4   // Force all textures to be treated as 4-component
    );
    nc = 4; // Force all textures to be treated as 4-component
    auto texture = vir::TextureBuffer2D::create
    (
        loadedData, 
        w,
        h,
        vir::TextureBuffer::defaultInternalFormat(nc)
    ); 
    stbi_image_free(loadedData);
    return this->set(texture);
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
    CALL_NATIVE_RESOURCE_FUNC_HETERO
    (
        setColorBufferWrapMode(index, mode), 
        setWrapMode(index, mode)
    )
}

void Resource::setMagFilterMode(vir::TextureBuffer::FilterMode mode)
{
    if (!valid()) return;
    CALL_NATIVE_RESOURCE_FUNC_HETERO
    (
        setColorBufferMagFilterMode(mode), 
        setMagFilterMode(mode)
    )
}

void Resource::setMinFilterMode(vir::TextureBuffer::FilterMode mode)
{
    if (!valid()) return;
    CALL_NATIVE_RESOURCE_FUNC_HETERO
    (
        setColorBufferMinFilterMode(mode), 
        setMinFilterMode(mode)
    )
}

}