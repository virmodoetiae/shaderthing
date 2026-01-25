#include "vpch.h"
#include "vgraphics/vcore/vuniform.h"
#include "vgraphics/vcore/vbuffers.h"

namespace vir
{

UniquePtr<Uniform> Uniform::create()
{
    return UniquePtr<Uniform>(new Uniform());
}

Uniform::Uniform()
{
    setType(type_);
}

Uniform::~Uniform()
{
    deleteValue();
    // Create a copy to loop over as the original is modified within the
    // removeUniform() method
    auto clientBuffers0 = clientBuffers_;
    for (auto& dub : clientBuffers0)
    {
        if (!dub.valid())
            continue;
        dub->removeUniform(weakFromThis());
    }
}

void Uniform::deleteValue(bool deleteCache)
{
    #define CASE_DELETE(type, cppType)                  \
        case Uniform::Type::type :                      \
            if (isArray)                                \
                delete[] static_cast<cppType*>(value);  \
            else                                        \
                delete static_cast<cppType*>(value);    \
            break;
    
    auto _delete = [](void*& value, Type type, bool isArray)
    {
        if (value == nullptr)
            return;
        switch(type)
        {
            CASE_DELETE(Bool, bool)
            CASE_DELETE(UInt, uint32_t)
            CASE_DELETE(Int, int)
            CASE_DELETE(Int2, glm::ivec2)
            CASE_DELETE(Int3, glm::ivec3)
            CASE_DELETE(Int4, glm::ivec4)
            CASE_DELETE(Float, float)
            CASE_DELETE(Float2, glm::vec2)
            CASE_DELETE(Float3, glm::vec3)
            CASE_DELETE(Float4, glm::vec4)
            CASE_DELETE(Mat3, glm::mat3)
            CASE_DELETE(Mat4, glm::mat4)
            case Uniform::Type::Sampler2D :
            case Uniform::Type::Image2D :
                delete static_cast<TextureBuffer2D*>(value);
                break;
            case Uniform::Type::Sampler3D :
            case Uniform::Type::Image3D :
                delete static_cast<TextureBuffer3D*>(value);
                break;
            case Uniform::Type::SamplerCube :
            case Uniform::Type::ImageCube :
                delete static_cast<CubeMapBuffer*>(value);
                break;
        }
        value = nullptr;
    };

    if (deleteCache)
        _delete(cache_, type_, false);
    if (isValueOwner_)
        _delete(value_, type_, valueArraySize_ > 1);
    value_ = nullptr;
}

// TODO 19/11/25 - check if convenient to update clientBuffers_ layouts here
void Uniform::setType
(
    Type type, 
    uint32_t valueArraySize,
    bool doNotReinitializeIfImageOrSampler
)
{
    if (value_ != nullptr && type == type_ && valueArraySize == valueArraySize_)
        return;
    if 
    (
        doNotReinitializeIfImageOrSampler && 
        (
            (type == Type::Image2D      && type_ == Type::Sampler2D)    || 
            (type == Type::Sampler2D    && type_ == Type::Image2D)      ||
            (type == Type::Image3D      && type_ == Type::Sampler3D)    || 
            (type == Type::Sampler3D    && type_ == Type::Image3D)      ||
            (type == Type::ImageCube    && type_ == Type::SamplerCube)  || 
            (type == Type::SamplerCube  && type_ == Type::ImageCube)
        )
    )
    {
        type_ = type;
        return;
    }

    if (value_ != nullptr)
        deleteValue(type != type_);
    isValueOwner_ = true;
    type_ = type;
    valueArraySize_ = valueArraySize;

    #define CASE_INITIALIZE(type, cppType)              \
        case Uniform::Type::type :                      \
            if (valueArraySize > 1)                     \
                value_ = new cppType[valueArraySize](); \
            else                                        \
                value_ = new cppType();                 \
            break;

    switch(type)
    {
        CASE_INITIALIZE(Bool, bool)
        CASE_INITIALIZE(UInt, uint32_t)
        CASE_INITIALIZE(Int, int)
        CASE_INITIALIZE(Int2, glm::ivec2)
        CASE_INITIALIZE(Int3, glm::ivec3)
        CASE_INITIALIZE(Int4, glm::ivec4)
        CASE_INITIALIZE(Float, float)
        CASE_INITIALIZE(Float2, glm::vec2)
        CASE_INITIALIZE(Float3, glm::vec3)
        CASE_INITIALIZE(Float4, glm::vec4)
        CASE_INITIALIZE(Mat3, glm::mat3)
        CASE_INITIALIZE(Mat4, glm::mat4)
        case Uniform::Type::Sampler2D :
        case Uniform::Type::Image2D :
        case Uniform::Type::Sampler3D :
        case Uniform::Type::Image3D :
        case Uniform::Type::SamplerCube :
        case Uniform::Type::ImageCube :
            break;
    }
}

void Uniform::setType
(
    Type type, 
    bool doNotReinitializeIfImageOrSampler
)
{
    setType(type, doNotReinitializeIfImageOrSampler, valueArraySize_);
}

void Uniform::submitToAllClientBuffers()
{
    for (auto& dub : clientBuffers_)
    {
        if (!dub.valid())
            continue; // TODO: handle in a more meaningful way
        dub->submitUniform(weakFromThis().get());
    }
}

void Uniform::markForSubmissionToAllClientBuffers()
{
    for (auto& dub : clientBuffers_)
    {
        if (!dub.valid())
            continue; // TODO: handle in a more meaningful way
        dub->markUniformForSubmission(weakFromThis().get());
    }
}

void Uniform::removeFromAllClientBuffers()
{
    for (auto& dub : clientBuffers_)
    {
        if (!dub.valid())
            continue; // TODO: handle in a more meaningful way
        dub->removeUniform(weakFromThis());
    }
}

}