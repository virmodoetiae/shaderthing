#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglbuffers.h"
#include "vgraphics/vcore/vopengl/vopenglmisc.h"

namespace vir
{

GLint OpenGLInternalFormat(TextureBuffer::InternalFormat internalFormat)
{
    switch (internalFormat)
    {
        case TextureBuffer::InternalFormat::R_UNI_8 :
            return GL_R8;
        case TextureBuffer::InternalFormat::R_UI_8 :
            return GL_R8UI;
        case TextureBuffer::InternalFormat::R_UI_32 :
            return GL_R32UI;
        case TextureBuffer::InternalFormat::R_SF_32 :
            return GL_R32F;
        case TextureBuffer::InternalFormat::RG_UNI_8 :
            return GL_RG8;
        case TextureBuffer::InternalFormat::RG_UI_8 :
            return GL_RG8UI;
        case TextureBuffer::InternalFormat::RG_UI_32 :
            return GL_RG32UI;
        case TextureBuffer::InternalFormat::RG_SF_32 :
            return GL_RG32F;
        case TextureBuffer::InternalFormat::RGB_UNI_8 :
            return GL_RGB8;
        case TextureBuffer::InternalFormat::RGB_UI_8 :
            return GL_RGB8UI;
        case TextureBuffer::InternalFormat::RGB_UI_32 :
            return GL_RGB32UI;
        case TextureBuffer::InternalFormat::RGB_SF_32 :
            return GL_RGB32F;
        case TextureBuffer::InternalFormat::RGBA_UNI_8 :
            return GL_RGBA8;
        case TextureBuffer::InternalFormat::RGBA_UI_8 :
            return GL_RGBA8UI;
        case TextureBuffer::InternalFormat::RGBA_UI_32 :
            return GL_RGBA32UI;
        case TextureBuffer::InternalFormat::RGBA_SF_32 :
            return GL_RGBA32F;
        case TextureBuffer::InternalFormat::Undefined :
            return 0;
    }
    return 0;
}

GLint OpenGLFormat(TextureBuffer::InternalFormat internalFormat)
{
    switch (internalFormat)
    {
        case TextureBuffer::InternalFormat::R_UNI_8 :
            return GL_RED;
        case TextureBuffer::InternalFormat::R_UI_8 :
            return GL_RED_INTEGER;
        case TextureBuffer::InternalFormat::R_UI_32 :
            return GL_RED_INTEGER;
        case TextureBuffer::InternalFormat::R_SF_32 :
            return GL_RED;
        case TextureBuffer::InternalFormat::RG_UNI_8 :
            return GL_RG;
        case TextureBuffer::InternalFormat::RG_UI_8 :
            return GL_RG_INTEGER;
        case TextureBuffer::InternalFormat::RG_UI_32 :
            return GL_RG_INTEGER;
        case TextureBuffer::InternalFormat::RG_SF_32 :
            return GL_RG;
        case TextureBuffer::InternalFormat::RGB_UNI_8 :
            return GL_RGB;
        case TextureBuffer::InternalFormat::RGB_UI_8 :
            return GL_RGB_INTEGER;
        case TextureBuffer::InternalFormat::RGB_UI_32 :
            return GL_RGB_INTEGER;
        case TextureBuffer::InternalFormat::RGB_SF_32 :
            return GL_RGB;
        case TextureBuffer::InternalFormat::RGBA_UNI_8 :
            return GL_RGBA;
        case TextureBuffer::InternalFormat::RGBA_UI_8 :
            return GL_RGBA_INTEGER;
        case TextureBuffer::InternalFormat::RGBA_UI_32 :
            return GL_RGBA_INTEGER;
        case TextureBuffer::InternalFormat::RGBA_SF_32 :
            return GL_RGBA;
        case TextureBuffer::InternalFormat::Undefined :
            return 0;
    }
    return 0;
}

GLint OpenGLType(TextureBuffer::InternalFormat internalFormat)
{
    switch (internalFormat)
    {
        case TextureBuffer::InternalFormat::R_UNI_8 :
        case TextureBuffer::InternalFormat::R_UI_8 :
        case TextureBuffer::InternalFormat::RG_UNI_8 :
        case TextureBuffer::InternalFormat::RG_UI_8 :
        case TextureBuffer::InternalFormat::RGB_UNI_8 :
        case TextureBuffer::InternalFormat::RGB_UI_8 :
        case TextureBuffer::InternalFormat::RGBA_UNI_8 :
        case TextureBuffer::InternalFormat::RGBA_UI_8 :
            return GL_UNSIGNED_BYTE;
        case TextureBuffer::InternalFormat::R_UI_32 :
        case TextureBuffer::InternalFormat::RG_UI_32 :
        case TextureBuffer::InternalFormat::RGB_UI_32 :
        case TextureBuffer::InternalFormat::RGBA_UI_32 :
            return GL_UNSIGNED_INT;
        case TextureBuffer::InternalFormat::R_SF_32 :
        case TextureBuffer::InternalFormat::RG_SF_32 :
        case TextureBuffer::InternalFormat::RGB_SF_32 :
        case TextureBuffer::InternalFormat::RGBA_SF_32 :
            return GL_FLOAT;
        case TextureBuffer::InternalFormat::Undefined :
            return 0;
    }
    return 0;
}

GLint OpenGLImageBindMode(TextureBuffer::ImageBindMode mode)
{
    switch (mode)
    {
        case TextureBuffer::ImageBindMode::ReadOnly :
            return GL_READ_ONLY;
        case TextureBuffer::ImageBindMode::WriteOnly :
            return GL_WRITE_ONLY;
        case TextureBuffer::ImageBindMode::ReadWrite :
            return GL_READ_WRITE;
    }
    return 0;
}

const std::unordered_map<TextureBuffer::WrapMode, GLint> wrapModeToGLint_ = 
{
    {TextureBuffer::WrapMode::ClampToBorder, GL_CLAMP_TO_BORDER},
    {TextureBuffer::WrapMode::ClampToEdge, GL_CLAMP_TO_EDGE},
    {TextureBuffer::WrapMode::Repeat, GL_REPEAT},
    {TextureBuffer::WrapMode::MirroredRepeat, GL_MIRRORED_REPEAT}
};

const std::unordered_map<uint32_t, GLint> wrapIndexToGLint_ = 
{
    {0, GL_TEXTURE_WRAP_S},
    {1, GL_TEXTURE_WRAP_T},
    {2, GL_TEXTURE_WRAP_R}
};

const std::unordered_map<TextureBuffer::FilterMode, GLint> filterModeToGLint_ = 
{
    {TextureBuffer::FilterMode::Nearest, GL_NEAREST},
    {TextureBuffer::FilterMode::Linear, GL_LINEAR},
    {TextureBuffer::FilterMode::NearestMipmapNearest,GL_NEAREST_MIPMAP_NEAREST},
    {TextureBuffer::FilterMode::NearestMipmapLinear, GL_NEAREST_MIPMAP_LINEAR},
    {TextureBuffer::FilterMode::LinearMipmapNearest, GL_LINEAR_MIPMAP_NEAREST},
    {TextureBuffer::FilterMode::LinearMipmapLinear, GL_LINEAR_MIPMAP_LINEAR}
};

//----------------------------------------------------------------------------//
// Texture2D buffer ----------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLTextureBuffer2D::OpenGLTextureBuffer2D
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
) : 
TextureBuffer2D(data, width, height, internalFormat)
{
    if (width*height == 0 || internalFormat == InternalFormat::Undefined)
        throw std::runtime_error
        (
            "OpenGLTextureBuffer2D - invalid dimensions or internal format"
        );
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    for (int i=0;i<2;i++)
        glTexParameteri
        (
            GL_TEXTURE_2D, 
            wrapIndexToGLint_.at(i), 
            wrapModeToGLint_.at(wrapModes_[i])
        );
    // Zoom in filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(magFilterMode_));
    // Zoom out filter
    internalFormat_ = internalFormat;
    if 
    (
        isInternalFormatUnsigned() &&
        (
            minFilterMode_ != TextureBuffer::FilterMode::Nearest &&
            minFilterMode_ != TextureBuffer::FilterMode::Linear
        )
    )
        minFilterMode_ = FilterMode::Linear;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(minFilterMode_));

    // Create texture
    GLint glFormat = OpenGLFormat(internalFormat);
    bool resetAlignment = false;
    if (glFormat != GL_RGBA && glFormat != GL_RGBA_INTEGER)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        resetAlignment = true;
    }
    else
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 4 Is default
    GLint glInternalFormat = OpenGLInternalFormat(internalFormat);
    glTexImage2D
    (
        GL_TEXTURE_2D, 
        0, 
        glInternalFormat, // GL_RGB8UI
        width, 
        height, 
        0, 
        glFormat, // GL_RGBA_INTEGER
        OpenGLType(internalFormat), // GL_UNSIGNED_BYTE
        data
    );
    if (resetAlignment)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    // Swizzling setting
    if (nChannels_ == 1) 
    {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    else if (nChannels_ == 2) 
    {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    else if (nChannels_ == 3)
    {
        GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    int maxLevel = std::floor(std::log2(std::min(width, height)));
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, maxLevel);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    width_ = width;
    height_ = height;
    nChannels_ = TextureBuffer::nChannels(internalFormat);
}

OpenGLTextureBuffer2D::~OpenGLTextureBuffer2D()
{
    glDeleteTextures(1, &id_);
}

void OpenGLTextureBuffer2D::setWrapMode
(
    uint32_t index,
    TextureBuffer::WrapMode mode
)
{
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri
    (
        GL_TEXTURE_2D, 
        wrapIndexToGLint_.at(index), 
        wrapModeToGLint_.at(mode)
    );
    wrapModes_[index] = mode;
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTextureBuffer2D::setMagFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri
    (
        GL_TEXTURE_2D, 
        GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(mode)
    );
    magFilterMode_ = mode;
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTextureBuffer2D::setMinFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    if // Setting a mipmap based filtering mode to an unsigned int texture
       // format will render it unusable/unwritable/unreadable (as long as a
       // mipmap based filter mode is set). Thus, prevent such filtering modes
       // to be set in such a scenario
    (
        isInternalFormatUnsigned() &&
        (
            mode != TextureBuffer::FilterMode::Nearest &&
            mode != TextureBuffer::FilterMode::Linear
        )
    )
        return;

    glBindTexture(GL_TEXTURE_2D, id_);
    if 
    (
        mode != TextureBuffer::FilterMode::Nearest &&
        mode != TextureBuffer::FilterMode::Linear
    )
        glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri
    (
        GL_TEXTURE_2D, 
        GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(mode)
    );
    minFilterMode_ = mode;
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTextureBuffer2D::bind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void OpenGLTextureBuffer2D::bindImage
(
    uint32_t unit, 
    uint32_t level, 
    ImageBindMode mode
)
{
    glBindImageTexture
    (
        unit, 
        id_, 
        level, 
        GL_FALSE, 
        0, 
        OpenGLImageBindMode(mode), 
        OpenGLInternalFormat(internalFormat_)
    );
}

void OpenGLTextureBuffer2D::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTextureBuffer2D::unbindImage()
{
    glBindImageTexture
    (
        0, 
        id_, 
        0, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        OpenGLInternalFormat(internalFormat_)
    );
}

#define READ_DATA(id, dataType, glDataType)                                 \
    unsigned int size = width_*height_*nChannels_;                          \
    GLint glFormat = OpenGLFormat(internalFormat_);                         \
    bool resetAlignment = false;                                            \
    if (glFormat != GL_RGBA && glFormat != GL_RGBA_INTEGER)                 \
    {                                                                       \
        glPixelStorei(GL_PACK_ALIGNMENT, 1);                                \
        resetAlignment = true;                                              \
    }                                                                       \
    else                                                                    \
        glPixelStorei(GL_PACK_ALIGNMENT, 4);                                \
    if (allocate)                                                           \
        data = new dataType[size];                                          \
    glBindTexture(GL_TEXTURE_2D, id);                                       \
    glGetTexImage(GL_TEXTURE_2D, 0, glFormat, glDataType, data);            \
    glBindTexture(GL_TEXTURE_2D, 0);                                        \
    if (resetAlignment)                                                     \
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

void OpenGLTextureBuffer2D::readData(unsigned char*& data, bool allocate)
{
    READ_DATA(id_, unsigned char, GL_UNSIGNED_BYTE)
}

void OpenGLTextureBuffer2D::readData(unsigned int*& data, bool allocate)
{
    READ_DATA(id_, unsigned int, GL_UNSIGNED_INT)
}

void OpenGLTextureBuffer2D::readData(float*& data, bool allocate)
{
    READ_DATA(id_, float, GL_FLOAT)
}

void OpenGLTextureBuffer2D::updateMipmap(bool onlyIfRequiredByFilterMode)
{
    if 
    (
        onlyIfRequiredByFilterMode &&
        (
            minFilterMode_ == FilterMode::Nearest ||
            minFilterMode_ == FilterMode::Linear
        )
    )
        return;
    glBindTexture(GL_TEXTURE_2D, id_);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//----------------------------------------------------------------------------//
// Animated 2D textue buffer -------------------------------------------------//
//----------------------------------------------------------------------------//

uint32_t OpenGLAnimatedTextureBuffer2D::nextFreeId_ = 0;

OpenGLAnimatedTextureBuffer2D::OpenGLAnimatedTextureBuffer2D
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nFrames,
    InternalFormat internalFormat
) : AnimatedTextureBuffer2D(data, width, height, nFrames, internalFormat)
{
    if (width*height == 0 || internalFormat == InternalFormat::Undefined)
        throw std::runtime_error
        (
            "OpenGLAnimatedTextureBuffer2D - invalid dimensions or internal format"
        );
    id_ = OpenGLAnimatedTextureBuffer2D::nextFreeId_++;
    width_ = width;
    height_ = height;
    nChannels_ = TextureBuffer::nChannels(internalFormat);
    internalFormat = internalFormat;
    auto frameSize = width*height*nChannels_;
    if (frames_.size() != nFrames)
    {
        for (uint32_t i=0; i<frames_.size(); i++)
            delete frames_[i];
        frames_.resize(nFrames);
    }
    for (uint32_t i=0; i<nFrames; i++)
    {
        frames_[i] = 
            new OpenGLTextureBuffer2D
            (
                &(data[i*frameSize]),
                width,
                height,
                internalFormat
            );
    }
    frameIndex_ = 0;
    frame_ = frames_[frameIndex_];
}

OpenGLAnimatedTextureBuffer2D::OpenGLAnimatedTextureBuffer2D
(
    std::vector<TextureBuffer2D*>& frames,
    bool gainFrameOwnership
) : AnimatedTextureBuffer2D(frames, gainFrameOwnership)
{}

OpenGLAnimatedTextureBuffer2D::~OpenGLAnimatedTextureBuffer2D()
{}

void OpenGLAnimatedTextureBuffer2D::setWrapMode
(
    uint32_t index,
    TextureBuffer::WrapMode mode
)
{
    for (auto* frame : frames_)
        frame->setWrapMode(index, mode);
    wrapModes_[index] = mode;
}

void OpenGLAnimatedTextureBuffer2D::setMagFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    for (auto* frame : frames_)
        frame->setMagFilterMode(mode);
    magFilterMode_ = mode;
}

void OpenGLAnimatedTextureBuffer2D::setMinFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    for (auto* frame : frames_)
        frame->setMinFilterMode(mode);
    minFilterMode_ = mode;
}

void OpenGLAnimatedTextureBuffer2D::bind(uint32_t unit)
{
    if (frame_ == nullptr)
        return;
    frame_->bind(unit);
}

void OpenGLAnimatedTextureBuffer2D::bindImage
(
    uint32_t unit, 
    uint32_t level, 
    ImageBindMode mode
)
{
    glBindImageTexture
    (
        unit, 
        id_, 
        level, 
        GL_FALSE, 
        0, 
        OpenGLImageBindMode(mode), 
        OpenGLInternalFormat(internalFormat_)
    );
}

void OpenGLAnimatedTextureBuffer2D::unbind()
{
    if (frame_ == nullptr)
        return;
    // This might fail if the frames of this animation consist of other 
    // TextureBuffer2Ds, as those might get deleted before this animation does.
    // Unfortunately (and also due to my unwillingless to use smart ptrs), there
    // is no way around this for now, but whatever
    try
    {
        frame_->unbind();
    }
    catch(...)
    {
    }
}

void OpenGLAnimatedTextureBuffer2D::unbindImage()
{
    glBindImageTexture
    (
        0, 
        id_, 
        0, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        OpenGLInternalFormat(internalFormat_)
    );
}

void OpenGLAnimatedTextureBuffer2D::readData(unsigned char*& data, bool allocate)
{
    READ_DATA(frame_->id(), unsigned char, GL_UNSIGNED_BYTE)
}

void OpenGLAnimatedTextureBuffer2D::readData(unsigned int*& data, bool allocate)
{
    READ_DATA(frame_->id(), unsigned int, GL_UNSIGNED_INT)
}

void OpenGLAnimatedTextureBuffer2D::readData(float*& data, bool allocate)
{
    READ_DATA(frame_->id(), float, GL_FLOAT)
}

void OpenGLAnimatedTextureBuffer2D::updateMipmap(bool onlyIfRequiredByFilterMode)
{
    // For simplicity, only regenerate the mipmap of the current frame
    if 
    (
        onlyIfRequiredByFilterMode &&
        (
            minFilterMode_ == FilterMode::Nearest ||
            minFilterMode_ == FilterMode::Linear
        )
    )
        return;
    frame_->updateMipmap();
}

//----------------------------------------------------------------------------//
// CubeMap buffer ------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLCubeMapBuffer::OpenGLCubeMapBuffer
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    if (internalFormat == InternalFormat::Undefined || width*height == 0)
        throw std::runtime_error
        (
            "OpenGLCubeMapBuffer - invalid dimensions or internal format"
        );
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    GLint glFormat = OpenGLFormat(internalFormat);
    bool resetAlignment = false;
    if (glFormat != GL_RGBA && glFormat != GL_RGBA_INTEGER)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        resetAlignment = true;
    }
    else
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 4 Is default
    GLint glInternalFormat = OpenGLInternalFormat(internalFormat);
    for (unsigned int i = 0; i < 6; i++)
        glTexImage2D
        (
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
            0, 
            glInternalFormat, 
            width, 
            height, 
            0, 
            glFormat, 
            OpenGLType(internalFormat), 
            faceData[i]
        );
    if (resetAlignment)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    // Zoom in filter
    magFilterMode_ = FilterMode::Linear;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(magFilterMode_));
    // Zoom out filter
    minFilterMode_ = FilterMode::Linear;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(minFilterMode_));
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, borderColor);
    for (int i=0;i<3;i++)
    {
        wrapModes_[i] = WrapMode::ClampToEdge;
        glTexParameteri
        (
            GL_TEXTURE_CUBE_MAP, 
            wrapIndexToGLint_.at(i), 
            wrapModeToGLint_.at(wrapModes_[i])
        );
    }
    int maxLevel = std::floor(std::log2(std::min(width, height)));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxLevel);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    width_ = width;
    height_ = height;
    nChannels_ = TextureBuffer::nChannels(internalFormat);
}

OpenGLCubeMapBuffer::~OpenGLCubeMapBuffer()
{
    glDeleteTextures(1, &id_);
}

void OpenGLCubeMapBuffer::bind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
}

void OpenGLCubeMapBuffer::bindImage
(
    uint32_t unit, 
    uint32_t level, 
    ImageBindMode mode
)
{
    glBindImageTexture
    (
        unit, 
        id_, 
        level, 
        GL_FALSE, 
        0, 
        OpenGLImageBindMode(mode), 
        OpenGLInternalFormat(internalFormat_)
    );
}

void OpenGLCubeMapBuffer::unbind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLCubeMapBuffer::unbindImage()
{
    glBindImageTexture
    (
        0, 
        id_, 
        0, 
        GL_FALSE, 
        0, 
        GL_READ_WRITE, 
        OpenGLInternalFormat(internalFormat_)
    );
}

void OpenGLCubeMapBuffer::setWrapMode
(
    uint32_t index,
    TextureBuffer::WrapMode mode
)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);    
    glTexParameteri
    (
        GL_TEXTURE_CUBE_MAP, 
        wrapIndexToGLint_.at(index), 
        wrapModeToGLint_.at(mode)
    );
    wrapModes_[index] = mode;
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLCubeMapBuffer::setMagFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    glTexParameteri
    (
        GL_TEXTURE_CUBE_MAP, 
        GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(mode)
    );
    magFilterMode_ = mode;
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLCubeMapBuffer::setMinFilterMode

(
    TextureBuffer::FilterMode mode
)
{
    if // Setting a mipmap based filtering mode to an unsigned int texture
       // format will render it unusable/unwritable/unreadable (as long as a
       // mipmap based filter mode is set). Thus, prevent such filtering modes
       // to be set in such a scenario
    (
        isInternalFormatUnsigned() &&
        (
            mode != TextureBuffer::FilterMode::Nearest &&
            mode != TextureBuffer::FilterMode::Linear
        )
    )
        return;
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    if 
    (
        mode != TextureBuffer::FilterMode::Nearest &&
        mode != TextureBuffer::FilterMode::Linear
    )
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri
    (
        GL_TEXTURE_CUBE_MAP, 
        GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(mode)
    );
    minFilterMode_ = mode;
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLCubeMapBuffer::readData(unsigned char*& data, bool allocate)
{
    (void)data;
    (void)allocate;
    throw std::runtime_error("OpenGLCubeMapBuffer::readData - Not implemented");
}

void OpenGLCubeMapBuffer::readData(unsigned int*& data, bool allocate)
{
    (void)data;
    (void)allocate;
    throw std::runtime_error("OpenGLCubeMapBuffer::readData - Not implemented");
}

void OpenGLCubeMapBuffer::readData(float*& data, bool allocate)
{
    (void)data;
    (void)allocate;
    throw std::runtime_error("OpenGLCubeMapBuffer::readData - Not implemented");
}

void OpenGLCubeMapBuffer::updateMipmap(bool onlyIfRequiredByFilterMode)
{
    if 
    (
        onlyIfRequiredByFilterMode &&
        (
            minFilterMode_ == FilterMode::Nearest ||
            minFilterMode_ == FilterMode::Linear
        )
    )
        return;
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

//----------------------------------------------------------------------------//
// Texture3D buffer ----------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLTextureBuffer3D::OpenGLTextureBuffer3D
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    InternalFormat internalFormat
) : 
TextureBuffer3D(data, width, height, depth, internalFormat)
{
    if (width*height*depth == 0 || internalFormat == InternalFormat::Undefined)
        throw std::runtime_error
        (
            "OpenGLTextureBuffer3D - invalid dimensions or internal format"
        );
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_3D, id_);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor);
    for (int i=0;i<3;i++)
        glTexParameteri
        (
            GL_TEXTURE_3D, 
            wrapIndexToGLint_.at(i), 
            wrapModeToGLint_.at(wrapModes_[i])
        );
    // Zoom in filter
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(magFilterMode_));
    // Zoom out filter
    internalFormat_ = internalFormat;
    if 
    (
        isInternalFormatUnsigned() &&
        (
            minFilterMode_ != TextureBuffer::FilterMode::Nearest &&
            minFilterMode_ != TextureBuffer::FilterMode::Linear
        )
    )
        minFilterMode_ = FilterMode::Linear;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(minFilterMode_));

    // Create texture
    GLint glFormat = OpenGLFormat(internalFormat);
    bool resetAlignment = false;
    if (glFormat != GL_RGBA && glFormat != GL_RGBA_INTEGER)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        resetAlignment = true;
    }
    else
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 4 Is default
    GLint glInternalFormat = OpenGLInternalFormat(internalFormat);
    glTexImage3D
    (
        GL_TEXTURE_3D, 
        0, 
        glInternalFormat, // GL_RGB8UI
        width, 
        height, 
        depth,
        0, 
        glFormat, // GL_RGBA_INTEGER
        OpenGLType(internalFormat), // GL_UNSIGNED_BYTE
        data
    );
    if (resetAlignment)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    // Swizzling setting
    if (nChannels_ == 1) 
    {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    else if (nChannels_ == 2) 
    {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
        glTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    else if (nChannels_ == 3)
    {
        GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ONE};
        glTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    int maxLevel = 
        std::floor(std::log2(std::min(std::min(width, height), depth)));
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, maxLevel);
    // Better not to generate mipmaps by default due to the weird observed RAM 
    // usage after they are created (magic driver issues...? Who knows?)
    //glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
    width_ = width;
    height_ = height;
    depth_ = depth;
    nChannels_ = TextureBuffer::nChannels(internalFormat);
}

OpenGLTextureBuffer3D::~OpenGLTextureBuffer3D()
{
    glDeleteTextures(1, &id_);
}

uint32_t OpenGLTextureBuffer3D::maxSideSize()
{
    GLint size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &size);
    return size;
}

void OpenGLTextureBuffer3D::setWrapMode
(
    uint32_t index,
    TextureBuffer::WrapMode mode
)
{
    glBindTexture(GL_TEXTURE_3D, id_);
    glTexParameteri
    (
        GL_TEXTURE_3D, 
        wrapIndexToGLint_.at(index), 
        wrapModeToGLint_.at(mode)
    );
    wrapModes_[index] = mode;
    glBindTexture(GL_TEXTURE_3D, 0);
}

void OpenGLTextureBuffer3D::setMagFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    glBindTexture(GL_TEXTURE_3D, id_);
    glTexParameteri
    (
        GL_TEXTURE_3D, 
        GL_TEXTURE_MAG_FILTER, 
        filterModeToGLint_.at(mode)
    );
    magFilterMode_ = mode;
    glBindTexture(GL_TEXTURE_3D, 0);
}

void OpenGLTextureBuffer3D::setMinFilterMode
(
    TextureBuffer::FilterMode mode
)
{
    if // Setting a mipmap based filtering mode to an unsigned int texture
       // format will render it unusable/unwritable/unreadable (as long as a
       // mipmap based filter mode is set). Thus, prevent such filtering modes
       // to be set in such a scenario
    (
        isInternalFormatUnsigned() &&
        (
            mode != TextureBuffer::FilterMode::Nearest &&
            mode != TextureBuffer::FilterMode::Linear
        )
    )
        return;

    glBindTexture(GL_TEXTURE_3D, id_);
    if 
    (
        mode != TextureBuffer::FilterMode::Nearest &&
        mode != TextureBuffer::FilterMode::Linear
    )
        glGenerateMipmap(GL_TEXTURE_3D);
    glTexParameteri
    (
        GL_TEXTURE_3D, 
        GL_TEXTURE_MIN_FILTER, 
        filterModeToGLint_.at(mode)
    );
    minFilterMode_ = mode;
    glBindTexture(GL_TEXTURE_3D, 0);
}

void OpenGLTextureBuffer3D::bind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_3D, id_);
}

void OpenGLTextureBuffer3D::bindImage
(
    uint32_t unit, 
    uint32_t level, 
    ImageBindMode mode
)
{
    glBindImageTexture
    (
        unit, 
        id_, 
        level, 
        GL_TRUE, // Layered 
        0, 
        OpenGLImageBindMode(mode), 
        OpenGLInternalFormat(internalFormat_)
    );
}

void OpenGLTextureBuffer3D::unbind()
{
    glBindTexture(GL_TEXTURE_3D, 0);
}

void OpenGLTextureBuffer3D::unbindImage()
{
    glBindImageTexture
    (
        0, 
        id_, 
        0, 
        GL_TRUE, // Layered 
        0, 
        GL_READ_WRITE, 
        OpenGLInternalFormat(internalFormat_)
    );
}

#define READ_DATA_3D(id, dataType, glDataType)                              \
    unsigned int size = width_*height_*depth_*nChannels_;                   \
    GLint glFormat = OpenGLFormat(internalFormat_);                         \
    bool resetAlignment = false;                                            \
    if (glFormat != GL_RGBA && glFormat != GL_RGBA_INTEGER)                 \
    {                                                                       \
        glPixelStorei(GL_PACK_ALIGNMENT, 1);                                \
        resetAlignment = true;                                              \
    }                                                                       \
    else                                                                    \
        glPixelStorei(GL_PACK_ALIGNMENT, 4);                                \
    if (allocate)                                                           \
        data = new dataType[size];                                          \
    glBindTexture(GL_TEXTURE_3D, id);                                       \
    glGetTexImage(GL_TEXTURE_3D, 0, glFormat, glDataType, data);            \
    glBindTexture(GL_TEXTURE_3D, 0);                                        \
    if (resetAlignment)                                                     \
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

void OpenGLTextureBuffer3D::readData(unsigned char*& data, bool allocate)
{
    READ_DATA_3D(id_, unsigned char, GL_UNSIGNED_BYTE)
}

void OpenGLTextureBuffer3D::readData(unsigned int*& data, bool allocate)
{
    READ_DATA_3D(id_, unsigned int, GL_UNSIGNED_INT)
}

void OpenGLTextureBuffer3D::readData(float*& data, bool allocate)
{
    READ_DATA_3D(id_, float, GL_FLOAT)
}

void OpenGLTextureBuffer3D::updateMipmap(bool onlyIfRequiredByFilterMode)
{
    if 
    (
        onlyIfRequiredByFilterMode &&
        (
            minFilterMode_ == FilterMode::Nearest ||
            minFilterMode_ == FilterMode::Linear
        )
    )
        return;
    glBindTexture(GL_TEXTURE_3D, id_);
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
}

//----------------------------------------------------------------------------//
// Framebuffer ---------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLFramebuffer::OpenGLFramebuffer
(
    uint32_t width, 
    uint32_t height,
    TextureBuffer::InternalFormat internalFormat
)
{
    width_ = width;
    height_ = height;

    // Create framebuffer
    glGenFramebuffers(1, &id_);
    glBindFramebuffer(GL_FRAMEBUFFER, id_);

    // Create color attachment texture
    colorBuffer_ = new OpenGLTextureBuffer2D
    (
        NULL, 
        width, 
        height, 
        internalFormat
    );
    colorBufferId_ = colorBuffer_->id();
    colorBuffer_->setMinFilterMode(TextureBuffer::FilterMode::Linear);
    glFramebufferTexture2D
    (
        GL_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0, 
        GL_TEXTURE_2D, 
        colorBufferId_, 
        0
    );

    // Create depth buffer
    glGenRenderbuffers(1, &depthBufferId_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferId_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer
    (
        GL_FRAMEBUFFER, 
        GL_DEPTH_STENCIL_ATTACHMENT, 
        GL_RENDERBUFFER, 
        depthBufferId_
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer setup invalid or incomplete");

    // All color attachments to which I can (but don't have to) draw
    //GLenum bufs[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, 
    //GL_COLOR_ATTACHMENT2};
    //glDrawBuffers(3, bufs);
    
    GLenum buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Init color attachment mipmaps
    updateColorBufferMipmap();
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
    if (activeOne_ == this)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        activeOne_ = nullptr;
    }
    glDeleteRenderbuffers(1, &depthBufferId_);
    //glDeleteTextures(1, &colorBufferId_);
    delete colorBuffer_;
    glDeleteFramebuffers(1, &id_);
}

void OpenGLFramebuffer::bind()
{
    if (activeOne_ != nullptr)
    {
        if (activeOne_ == this)
            return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
    activeOne_ = this;
}

void OpenGLFramebuffer::unbind()
{
    if (activeOne_ == nullptr)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    activeOne_ = nullptr;
}

void OpenGLFramebuffer::bindColorBuffer(uint32_t unit)
{
    colorBuffer_->bind(unit);
}

void OpenGLFramebuffer::bindColorBufferToImage
(
    uint32_t unit, 
    uint32_t level, 
    TextureBuffer::ImageBindMode mode
)
{
    colorBuffer_->bindImage(unit, level, mode);
}

void OpenGLFramebuffer::unbindColorBuffer()
{
    colorBuffer_->unbind();
}

void OpenGLFramebuffer::unbindColorBufferFromImage()
{
    colorBuffer_->unbindImage();
}

void OpenGLFramebuffer::bindDepthBuffer(uint32_t unit)
{
    (void)unit;
    /*glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, depthBufferId_);*/
    //glBindRenderbuffer(GL_RENDERBUFFER, depthBufferId_);
}

template <typename DataType>
void OpenGLFramebuffer::readColorBufferData
(
    DataType*& data, 
    bool yFlip, 
    bool allocate, 
    GLint glDataType
)
{
    if (allocate)
        data = new DataType[colorBufferDataSize()];
    bind();
    GLint glFormat = OpenGLFormat(colorBuffer_->internalFormat());
    bool resetAlignment = false;
    if (glFormat != GL_RGBA && glFormat != GL_RGBA_INTEGER)
    {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        resetAlignment = true;
    }
    else
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels
    (
        0, 
        0, 
        width_, 
        height_, 
        OpenGLFormat(colorBuffer_->internalFormat()), 
        glDataType, 
        data
    );
    if (resetAlignment)
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (yFlip)
        for(uint32_t line = 0; line != height_/2; ++line)
        {
            std::swap_ranges
            (
                &data[0] + colorBuffer_->nChannels() * width_ * line,
                &data[0] + colorBuffer_->nChannels() * width_ * (line+1),
                &data[0] + colorBuffer_->nChannels() * width_ * (height_-line-1)
            );
        }
}

void OpenGLFramebuffer::readColorBufferData
(
    unsigned char*& data, 
    bool yFlip, 
    bool allocate
)
{
    readColorBufferData(data, yFlip, allocate, GL_UNSIGNED_BYTE);
}

void OpenGLFramebuffer::readColorBufferData
(
    unsigned int*& data, 
    bool yFlip, 
    bool allocate
)
{
    readColorBufferData(data, yFlip, allocate, GL_UNSIGNED_INT);
}

void OpenGLFramebuffer::readColorBufferData
(
    float*& data, 
    bool yFlip, 
    bool allocate
)
{
    readColorBufferData(data, yFlip, allocate, GL_FLOAT);
}

void OpenGLFramebuffer::clearColorBuffer(float r, float g, float b, float a)
{
    auto* previouslyActiveFramebuffer = activeOne_;
    bind();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    if (previouslyActiveFramebuffer != nullptr)
        previouslyActiveFramebuffer->bind();
}

void OpenGLFramebuffer::updateColorBufferMipmap(bool onlyIfRequiredByFilterMode)
{
    colorBuffer_->updateMipmap(onlyIfRequiredByFilterMode);
}

//----------------------------------------------------------------------------//
// Uniform buffer ------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size) :
UniformBuffer(size)
{
    glGenBuffers(1, &id_);
    glBindBuffer(GL_UNIFORM_BUFFER, id_);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
}

OpenGLUniformBuffer::~OpenGLUniformBuffer()
{
    glDeleteBuffers(1, &id_);
}

void OpenGLUniformBuffer::bind()
{
    glBindBuffer(GL_UNIFORM_BUFFER, id_);
}

void OpenGLUniformBuffer::unbind()
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLUniformBuffer::setBindingPoint(uint32_t bindingPoint)
{
    glBindBuffer(GL_UNIFORM_BUFFER, id_);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id_);
}

void OpenGLUniformBuffer::setData
(
    void* data,
    uint32_t size,
    uint32_t offset
)
{
    glBufferSubData
    (
        GL_UNIFORM_BUFFER, 
        offset, 
        size == 0 ? size_ : size, 
        data
    );
}

//----------------------------------------------------------------------------//
// Shader storage buffer -----------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(uint32_t size) :
ShaderStorageBuffer(size)
{
    glGenBuffers(1, &id_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
    //glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    glBufferStorage
    (
        GL_SHADER_STORAGE_BUFFER, 
        size,
        NULL,
        GL_MAP_PERSISTENT_BIT | 
        GL_MAP_COHERENT_BIT |
        GL_MAP_WRITE_BIT | 
        GL_MAP_READ_BIT
    );
}

OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, id_);
    glDeleteBuffers(1, &id_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

bool OpenGLShaderStorageBuffer::canRunOnDeviceInUse() const
{
    // Requires OpenGL >= 4.3
    return
        (
            Window::instance()->context()->versionMajor() == 4 &&
            Window::instance()->context()->versionMinor() >= 3
        ) ||
        Window::instance()->context()->versionMajor() > 4;
}

void OpenGLShaderStorageBuffer::bind()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
}

void OpenGLShaderStorageBuffer::unbind()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void OpenGLShaderStorageBuffer::setBindingPoint(uint32_t bindingPoint)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, id_);
}

bool OpenGLShaderStorageBuffer::isBoundToBindingPoint
(
    uint32_t bindingPoint
) const
{
    GLint boundBuffer = 0;
    glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, bindingPoint, &boundBuffer);
    return boundBuffer == (GLint)id_;
}

void* OpenGLShaderStorageBuffer::mapData
(
    uint32_t size,
    uint32_t offset
)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    return glMapBufferRange
    (
        GL_SHADER_STORAGE_BUFFER, 
        offset, 
        size == 0 ? size_ : size,
        GL_MAP_PERSISTENT_BIT | 
        GL_MAP_COHERENT_BIT |
        GL_MAP_WRITE_BIT | 
        GL_MAP_READ_BIT
    );
}

// Wait for all shader invocations writing to this SSBO to finish writing
void OpenGLShaderStorageBuffer::memoryBarrier()
{
    glMemoryBarrier
    (
        GL_BUFFER_UPDATE_BARRIER_BIT | 
        GL_SHADER_STORAGE_BARRIER_BIT |
        GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
    );
}

void OpenGLShaderStorageBuffer::fenceSync()
{
    OpenGLWaitSync();
}

//----------------------------------------------------------------------------//
// Vertex buffer -------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
{
    count_ = size/((uint32_t)sizeof(float));
    glGenBuffers(1, &id_);
    glBindBuffer(GL_ARRAY_BUFFER, id_);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindVertexBuffer(this);
    }
    glDeleteBuffers(1, &id_);
}

void OpenGLVertexBuffer::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, id_);
    VertexBuffer::setLayout();
}

void OpenGLVertexBuffer::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLVertexBuffer::setLayout
(
    const VertexBufferLayout& layout
)
{
    auto GLType = [](const VertexBufferLayout::Element& e)
    {
        switch(e.variable.type)
        {
            case (Shader::Variable::Type::Bool) :
                return GL_BOOL;
            case (Shader::Variable::Type::UInt) :
                return GL_UNSIGNED_BYTE;
            case (Shader::Variable::Type::Int) :
            case (Shader::Variable::Type::Int2) :
            case (Shader::Variable::Type::Int3) :
            case (Shader::Variable::Type::Int4) :
                return GL_INT;
            case (Shader::Variable::Type::Float) :
            case (Shader::Variable::Type::Float2) :
            case (Shader::Variable::Type::Float3) :
            case (Shader::Variable::Type::Float4) :
            case (Shader::Variable::Type::Mat3) :
            case (Shader::Variable::Type::Mat4) :
                return GL_FLOAT;
            case (Shader::Variable::Type::Sampler2D) :
            case (Shader::Variable::Type::Sampler3D) :
            case (Shader::Variable::Type::SamplerCube) :
            case (Shader::Variable::Type::Image2D) :
            case (Shader::Variable::Type::Image3D) :
            case (Shader::Variable::Type::ImageCube) :
                throw std::runtime_error("Invalid vertex buffer element type");
        }
        return GL_FLOAT;
    };

    uint32_t stride(layout.stride());
    for(const VertexBufferLayout::Element& e : layout.elements())
    {
        glVertexAttribPointer
        (   
            e.location, 
            e.variable.nCmpts, 
            GLType(e), 
            e.normalized? GL_TRUE : GL_FALSE, 
            stride, 
            (void*)(intptr_t)e.offset
        );
        glEnableVertexAttribArray(e.location);
    }
}

void OpenGLVertexBuffer::updateVertices(float* vertices, uint32_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, id_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}

//----------------------------------------------------------------------------//
// Index buffer --------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t size)
{
    count_ = size/((uint32_t)sizeof(uint32_t));
    glGenBuffers(1, &id_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindIndexBuffer();
    }
    glDeleteBuffers(1, &id_);
}

void OpenGLIndexBuffer::bind() 
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
}

void OpenGLIndexBuffer::unbind() 
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//----------------------------------------------------------------------------//
// Vertex array --------------------------------------------------------------//
//----------------------------------------------------------------------------//

OpenGLVertexArray::OpenGLVertexArray()
{
    glBindVertexArray(0);
    glGenVertexArrays(1, &id_);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
    for (auto& v : vertexBuffers_)
    {
        unbindVertexBuffer(v);
    }
    unbindIndexBuffer();
    glDeleteVertexArrays(1, &id_);
}

void OpenGLVertexArray::bind()
{
    if (activeOne_ != nullptr)
    {
        if (activeOne_ == this)
            return;
    }
    glBindVertexArray(id_);
    activeOne_ = this;
}

void OpenGLVertexArray::unbind()
{
    if (activeOne_ == nullptr)
        return;
    glBindVertexArray(0);
    activeOne_ = nullptr;
}

void OpenGLVertexArray::bindVertexBuffer(VertexBuffer* vertexBuffer)
{
    // When bidning a vertex buffer to this vertex array, first check if the 
    // buffer was already registered. If not, register it and also register the
    // array with the buffer
    auto it = Helpers::findInPtrVector(vertexBuffer, vertexBuffers_);
    if (it != vertexBuffers_.end()) // If found
        return;
    vertexBuffers_.push_back(vertexBuffer);
    vertexBuffer->vertexArrays().push_back(this);
    
    bind();
    vertexBuffer->bind();
}

void OpenGLVertexArray::unbindVertexBuffer(VertexBuffer* vertexBuffer)
{
    // When unregistering a vertex buffer from this array, first check if the
    // buffer was already registered. If so, unregister it, and also unregister
    // the array with the buffer
    auto it = Helpers::findInPtrVector(vertexBuffer, vertexBuffers_);
    if (it == vertexBuffers_.end()) // If not found
        return;
    vertexBuffers_.erase(it);
    auto it2 = Helpers::findInPtrVector<VertexArray>
    (
        this, vertexBuffer->vertexArrays()
    );
    if (it2 != vertexBuffer->vertexArrays().end()) // If found
        vertexBuffer->vertexArrays().erase(it2);
    
    bind();
    vertexBuffer->unbind();
}

void OpenGLVertexArray::bindIndexBuffer(IndexBuffer* indexBuffer)
{
    if (indexBuffer == nullptr)
        return;
    indexBuffer->vertexArrays().push_back(this);
    bind();
    indexBuffer->bind();
    indexBuffer_ = indexBuffer;
}

void OpenGLVertexArray::unbindIndexBuffer()
{
    if (indexBuffer_ == nullptr)
        return;
    auto it = Helpers::findInPtrVector<VertexArray>
    (
        this, indexBuffer_->vertexArrays()
    );
    if (it != indexBuffer_->vertexArrays().end()) // If found
        indexBuffer_->vertexArrays().erase(it);
    bind();
    indexBuffer_->unbind();
    indexBuffer_ = nullptr;
}

}