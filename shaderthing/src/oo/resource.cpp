#include "vir/include/vpch.h"
#include "shaderthing/include/oo/resource.h"
#include "shaderthing/include/oo/objectio.h"
#include "shaderthing/include/helpers.h"

namespace ShaderThing
{

const std::map<Resource::Type, const char*> Resource::typeToName =
{
    {Resource::Type::Texture2D,         "Texture-2D"},
    {Resource::Type::Texture3D,         "Texture-3D"},
    {Resource::Type::AnimatedTexture2D, "Animation-2D"},
    {Resource::Type::Cubemap,           "Cubemap"},
    {Resource::Type::Framebuffer,       "Layer"}
};

Resource::~Resource()
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
}

std::string Resource::name() const 
{
    return namePtr_ == nullptr? "" : *namePtr_;
}

void Resource::setName(const std::string& name)
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
    namePtr_ = new std::string(name);
    isNameManaged_ = false;
}

void Resource::setName(std::string* namePtr)
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
    namePtr_ = namePtr;
    isNameManaged_ = true;
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

template <typename NativeType>
CRTPResource<NativeType>::~CRTPResource()
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
    if (native_ != nullptr)
    {
        if (textureUnit_ != -1)
            this->unbind();
        if (imageUnit_ != -1)
            this->unbindImage();
    }
    if (rawData_ != nullptr) 
        delete[] rawData_;
}

void Texture2DResource::save(ObjectIO& io)
{
    io.writeObjectStart(namePtr_->c_str());
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

}