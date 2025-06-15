#include "vpch.h"
#include <cmath>

#include "vgraphics/vcore/vopengl/vopenglbuffers.h"

#include "thirdparty/stb/stb_image.h"

namespace vir
{

// Buffer base ---------------------------------------------------------------//

// Texture base --------------------------------------------------------------//

const std::unordered_map<TextureBuffer::InternalFormat, std::string> 
    TextureBuffer::internalFormatToName =
{
    {TextureBuffer::InternalFormat::Undefined, "Undefined"},
    {TextureBuffer::InternalFormat::R_UNI_8, "R uint 8-bit norm."},
    {TextureBuffer::InternalFormat::R_UI_8, "R uint 8-bit"},
    {TextureBuffer::InternalFormat::R_UI_32, "R uint 32-bit"},
    {TextureBuffer::InternalFormat::R_SF_32, "R float 32-bit"},
    {TextureBuffer::InternalFormat::RG_UNI_8, "RG uint 8-bit norm."},
    {TextureBuffer::InternalFormat::RG_UI_8, "RG uint 8-bit"},
    {TextureBuffer::InternalFormat::RG_UI_32, "RG uint 32-bit"},
    {TextureBuffer::InternalFormat::RG_SF_32, "RG float 32-bit"},
    {TextureBuffer::InternalFormat::RGB_UNI_8, "RGB uint 8-bit norm."},
    {TextureBuffer::InternalFormat::RGB_UI_8, "RGB uint 8-bit"},
    {TextureBuffer::InternalFormat::RGB_UI_32, "RGB uint 32-bit"},
    {TextureBuffer::InternalFormat::RGB_SF_32, "RGB float 32-bit"},
    {TextureBuffer::InternalFormat::RGBA_UNI_8, "RGBA uint 8-bit norm."},
    {TextureBuffer::InternalFormat::RGBA_UI_8, "RGBA uint 8-bit"},
    {TextureBuffer::InternalFormat::RGBA_UI_32, "RGBA uint 32-bit"},
    {TextureBuffer::InternalFormat::RGBA_SF_32, "RGBA float 32-bit"},
};

const std::unordered_map<TextureBuffer::InternalFormat, std::string> 
    TextureBuffer::internalFormatToShortName =
{
    {TextureBuffer::InternalFormat::Undefined, "undefined"},
    {TextureBuffer::InternalFormat::R_UNI_8, "r8"},
    {TextureBuffer::InternalFormat::R_UI_8, "r8ui"},
    {TextureBuffer::InternalFormat::R_UI_32, "r32ui"},
    {TextureBuffer::InternalFormat::R_SF_32, "r32f"},
    {TextureBuffer::InternalFormat::RG_UNI_8, "rg8"},
    {TextureBuffer::InternalFormat::RG_UI_8, "rg8ui"},
    {TextureBuffer::InternalFormat::RG_UI_32, "rg32ui"},
    {TextureBuffer::InternalFormat::RG_SF_32, "rg32f"},
    {TextureBuffer::InternalFormat::RGB_UNI_8, "rgba8"}, // rgb8 does not exist
    {TextureBuffer::InternalFormat::RGB_UI_8, "rgba8ui"}, // rgb8ui does not exist
    {TextureBuffer::InternalFormat::RGB_UI_32, "rgba32ui"}, // rgb32ui does not exist
    {TextureBuffer::InternalFormat::RGB_SF_32, "rgba32f"}, // rgb32f does not exist
    {TextureBuffer::InternalFormat::RGBA_UNI_8, "rgba8"},
    {TextureBuffer::InternalFormat::RGBA_UI_8, "rgba8ui"},
    {TextureBuffer::InternalFormat::RGBA_UI_32, "rgba32ui"},
    {TextureBuffer::InternalFormat::RGBA_SF_32, "rgba32f"},
};

const std::unordered_map<TextureBuffer::InternalFormat, bool> 
    TextureBuffer::internalFormatToIsUnsigned =
{
    {TextureBuffer::InternalFormat::Undefined, false},
    {TextureBuffer::InternalFormat::R_UNI_8, false}, // Normalized behaves like float
    {TextureBuffer::InternalFormat::R_UI_8, true},
    {TextureBuffer::InternalFormat::R_UI_32, true},
    {TextureBuffer::InternalFormat::R_SF_32, false},
    {TextureBuffer::InternalFormat::RG_UNI_8, false}, // Normalized behaves like float
    {TextureBuffer::InternalFormat::RG_UI_8, true},
    {TextureBuffer::InternalFormat::RG_UI_32, true},
    {TextureBuffer::InternalFormat::RG_SF_32, false},
    {TextureBuffer::InternalFormat::RGB_UNI_8, false}, // Normalized behaves like float
    {TextureBuffer::InternalFormat::RGB_UI_8, true},
    {TextureBuffer::InternalFormat::RGB_UI_32, true},
    {TextureBuffer::InternalFormat::RGB_SF_32, false},
    {TextureBuffer::InternalFormat::RGBA_UNI_8, false}, // Normalized behaves like float
    {TextureBuffer::InternalFormat::RGBA_UI_8, true},
    {TextureBuffer::InternalFormat::RGBA_UI_32, true},
    {TextureBuffer::InternalFormat::RGBA_SF_32, false},
};

const std::unordered_map<TextureBuffer::InternalFormat, TextureBuffer::DataType> 
    TextureBuffer::internalFormatToDataType =
{
    {TextureBuffer::InternalFormat::Undefined, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::R_UNI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::R_UI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::R_UI_32, TextureBuffer::DataType::UnsignedInt},
    {TextureBuffer::InternalFormat::R_SF_32, TextureBuffer::DataType::Float},
    {TextureBuffer::InternalFormat::RG_UNI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::RG_UI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::RG_UI_32, TextureBuffer::DataType::UnsignedInt},
    {TextureBuffer::InternalFormat::RG_SF_32, TextureBuffer::DataType::Float},
    {TextureBuffer::InternalFormat::RGB_UNI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::RGB_UI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::RGB_UI_32, TextureBuffer::DataType::UnsignedInt},
    {TextureBuffer::InternalFormat::RGB_SF_32, TextureBuffer::DataType::Float},
    {TextureBuffer::InternalFormat::RGBA_UNI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::RGBA_UI_8, TextureBuffer::DataType::UnsignedChar},
    {TextureBuffer::InternalFormat::RGBA_UI_32, TextureBuffer::DataType::UnsignedInt},
    {TextureBuffer::InternalFormat::RGBA_SF_32, TextureBuffer::DataType::Float},
};

const std::unordered_map<TextureBuffer::InternalFormat, uint32_t> 
    TextureBuffer::internalFormatToBytes =
{
    {TextureBuffer::InternalFormat::Undefined,  0},
    {TextureBuffer::InternalFormat::R_UNI_8,    1},
    {TextureBuffer::InternalFormat::R_UI_8,     1},
    {TextureBuffer::InternalFormat::R_UI_32,    4},
    {TextureBuffer::InternalFormat::R_SF_32,    4},
    {TextureBuffer::InternalFormat::RG_UNI_8,   2},
    {TextureBuffer::InternalFormat::RG_UI_8,    2},
    {TextureBuffer::InternalFormat::RG_UI_32,   8},
    {TextureBuffer::InternalFormat::RG_SF_32,   8},
    {TextureBuffer::InternalFormat::RGB_UNI_8,  3},
    {TextureBuffer::InternalFormat::RGB_UI_8,   3},
    {TextureBuffer::InternalFormat::RGB_UI_32,  12},
    {TextureBuffer::InternalFormat::RGB_SF_32,  12},
    {TextureBuffer::InternalFormat::RGBA_UNI_8, 4},
    {TextureBuffer::InternalFormat::RGBA_UI_8,  4},
    {TextureBuffer::InternalFormat::RGBA_UI_32, 16},
    {TextureBuffer::InternalFormat::RGBA_SF_32, 16},
};

const std::unordered_map<TextureBuffer::WrapMode, std::string> 
    TextureBuffer::wrapModeToName =
{
    {TextureBuffer::WrapMode::MirroredRepeat, "Mirror & repeat"},
    {TextureBuffer::WrapMode::Repeat, "Repeat"},
    {TextureBuffer::WrapMode::ClampToEdge, "Clamp to edge"},
    {TextureBuffer::WrapMode::ClampToBorder, "Clamp to border"}
};

const std::unordered_map<TextureBuffer::FilterMode, std::string> 
    TextureBuffer::filterModeToName =
{
    {TextureBuffer::FilterMode::LinearMipmapLinear, "Linear, mipmap-linear"},
    {TextureBuffer::FilterMode::NearestMipmapLinear, "Nearest, mipmap-linear"},
    {TextureBuffer::FilterMode::LinearMipmapNearest, "Linear, mipmap-nearest"},
    {TextureBuffer::FilterMode::NearestMipmapNearest,"Nearest, mipmap-nearest"},
    {TextureBuffer::FilterMode::Linear, "Linear"},
    {TextureBuffer::FilterMode::Nearest, "Nearest"}
};

#define ENFORCE_CHANNEL_FORMAT_CONSISTENCY                                  \
    if (internalFormat != InternalFormat::Undefined)                        \
        nChannels = TextureBuffer2D::nChannels(internalFormat);             \
    else                                                                    \
        internalFormat = TextureBuffer2D::defaultInternalFormat(nChannels); \

// Texture2D -----------------------------------------------------------------//

UniquePtr<TextureBuffer2D> TextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<TextureBuffer2D>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLTextureBuffer2D>
                (
                    data, 
                    width, 
                    height,
                    internalFormat
                );
        }
    }
    catch(...){}
    return nullUniquePtr<TextureBuffer2D>();
}

UniquePtr<TextureBuffer2D> TextureBuffer2D::create
(
    const unsigned char* fileData, 
    uint32_t size,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<TextureBuffer2D>();
    unsigned char* data = nullptr;
    UniquePtr<TextureBuffer2D> buffer;
    try
    {
        int width, height, nChannels;
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load_from_memory
        (
            fileData, 
            size, 
            &width, 
            &height, 
            &nChannels,
            TextureBuffer2D::nChannels(internalFormat)
        );
        if (fileData == nullptr || data == nullptr)
            throw std::runtime_error
            (
                "TextureBuffer2D::create - Invalid data"
            );
        ENFORCE_CHANNEL_FORMAT_CONSISTENCY
    
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                buffer = makeUnique<OpenGLTextureBuffer2D>
                (
                    data, 
                    width, 
                    height,
                    internalFormat
                );
        }
        stbi_image_free(data);
    }
    catch (...)
    {
        if (data != nullptr)
            stbi_image_free(data);
    }
    return buffer;
}

UniquePtr<TextureBuffer2D> TextureBuffer2D::create
(
    std::string filepath, 
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<TextureBuffer2D>();
    unsigned char* data = nullptr;
    UniquePtr<TextureBuffer2D> buffer;
    try
    {
        int width = 0, height = 0, nChannels = 0;
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load
        (
            filepath.c_str(), 
            &width, 
            &height, 
            &nChannels, 
            TextureBuffer2D::nChannels(internalFormat)
        );
        if (data == nullptr)
            throw std::runtime_error
            (
                "TextureBuffer2D::create - Invalid data in "+filepath
            );
        ENFORCE_CHANNEL_FORMAT_CONSISTENCY
    
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                buffer = makeUnique<OpenGLTextureBuffer2D>
                (
                    data, 
                    width, 
                    height,
                    internalFormat
                );
        }
        stbi_image_free(data);
    }
    catch (...)
    {
        if (data != nullptr)
            stbi_image_free(data);
    }
    return buffer;
}

uint64_t TextureBuffer2D::maxMemoryFootprint() const
{
    return (uint64_t)width_*
            (uint64_t)height_*
            (uint64_t)internalFormatToBytes.at(internalFormat_)*
            (4./3.); // Account for mipmaps (in 2D, the mipmaps are 1./3. of
                     // of the total base texture size as it converges like
                     // 1/(2^n)^2)
}

// AnimatedTexture2D ---------------------------------------------------------//

AnimatedTextureBuffer2D::AnimatedTextureBuffer2D() : 
TextureBuffer2D(),
time_(0),
frameIndex_(0),
frame_(nullptr),
frames_(0),
isFrameOwner_(true),
frameDuration_(1.0f/60)
{}

AnimatedTextureBuffer2D::AnimatedTextureBuffer2D
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nFrames,
    InternalFormat internalFormat
) :
TextureBuffer2D(data, width, height, internalFormat),
time_(0),
frameIndex_(0),
frame_(nullptr),
frames_(0),
isFrameOwner_(true),
frameDuration_(1.0f/60)
{
    frames_.resize(nFrames);
}

AnimatedTextureBuffer2D::AnimatedTextureBuffer2D
(
    std::vector<TextureBuffer2D*>& frames,
    bool gainFrameOwnership
) :
TextureBuffer2D(),
time_(0),
frameIndex_(0),
frame_(nullptr),
frames_(frames),
isFrameOwner_(gainFrameOwnership),
frameDuration_(1.0f/60)
{
    if (frames.size() == 0)
        throw std::runtime_error
        (
R"(vbuffers.cpp - AnimatedTextureBuffer2D(std::vector<TextureBuffer2D*>&, bool) 
- Cannot construct from empty array of frames)"
        );
    // Ensure consistency between frames
    bool firstFrame = true;
    for (auto* frame : frames_)
    {
        if (firstFrame)
        {
            width_ = frame->width();
            height_ = frame->height();
            internalFormat_ = frame->internalFormat();
            firstFrame = false;
        }
        else if 
        (
            frame->width() != width_ || 
            frame->height() != height_ || 
            frame->internalFormat() != internalFormat_
        )
        {
            throw std::runtime_error(
R"(vbuffers.cpp - AnimatedTextureBuffer2D(std::vector<TextureBuffer2D*>&, bool) 
- Cannot construct because not all frames have same width, height or internal 
format)"
            );
        }
    }
    frame_ = frames_[frameIndex_];
}

AnimatedTextureBuffer2D::~AnimatedTextureBuffer2D()
{
    frame_ = nullptr;
    if (!isFrameOwner_)
        return;
    for (auto* frame : frames_)
    {
        delete frame;
        frame = nullptr;
    }
    frames_.resize(0);
}

UniquePtr<AnimatedTextureBuffer2D> AnimatedTextureBuffer2D::create
(
    std::string filepath, 
    InternalFormat internalFormat
)
{
    unsigned char* fileData = nullptr;
    uint32_t size = 0;
    try
    {
        std::ifstream fileDataStream
        (
            filepath, std::ios::binary | std::ios::in
        );
        fileDataStream.seekg(0, std::ios::end);
        size = fileDataStream.tellg();
        fileDataStream.seekg(0, std::ios::beg);
        fileData = new unsigned char[size];
        fileDataStream.read((char*)fileData, size);
        fileDataStream.close();
    }
    catch(...)
    {
        if (fileData != nullptr)
            delete[] fileData;
        return nullUniquePtr<AnimatedTextureBuffer2D>();
    }
    auto buffer = AnimatedTextureBuffer2D::create
    (
        fileData,
        size,
        internalFormat
    );
    if (fileData != nullptr)
        delete[] fileData;
    return buffer;
}

UniquePtr<AnimatedTextureBuffer2D> AnimatedTextureBuffer2D::create
(
    const unsigned char* fileData, 
    uint32_t size,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<AnimatedTextureBuffer2D>();
    unsigned char* data = nullptr;
    int* delays = nullptr;
    UniquePtr<AnimatedTextureBuffer2D> buffer;
    try
    {
        // Unpack raw gif data in memory
        int width = 0, height = 0, nFrames = 0, nChannels = 0;
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load_gif_from_memory
        (
            fileData,
            size,
            &delays, // in ms, might be 0
            &width,
            &height,
            &nFrames,
            &nChannels,
            TextureBuffer::nChannels(internalFormat)
        );
        if (fileData == nullptr || data == nullptr)
            throw std::runtime_error
            (
                "AnimatedTextureBuffer2D::create - Invalid data"
            );
        ENFORCE_CHANNEL_FORMAT_CONSISTENCY
        
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                buffer = makeUnique<OpenGLAnimatedTextureBuffer2D>
                (
                    data,
                    width,
                    height,
                    nFrames,
                    internalFormat
                );
        }
        // Set overall duration (individual frame durations are not currently
        // stored)
        float duration = 0.f;
        for (int i=0; i<nFrames ;i++)
            duration += std::max(float(delays[i]/1000.0f), 0.01f);
        buffer->setDuration(duration);
        
        if (data != nullptr)
            stbi_image_free(data);
        if (delays != nullptr)
            stbi_image_free(delays);
    }
    catch(...)
    {
        if (data != nullptr)
            stbi_image_free(data);
        if (delays != nullptr)
            stbi_image_free(delays);
    }
    return buffer;
}

UniquePtr<AnimatedTextureBuffer2D> AnimatedTextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nFrames,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<AnimatedTextureBuffer2D>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLAnimatedTextureBuffer2D>
                (
                    data, 
                    width, 
                    height,
                    nFrames,
                    internalFormat
                );
        }
    }
    catch(...){}
    return nullUniquePtr<AnimatedTextureBuffer2D>();
}

UniquePtr<AnimatedTextureBuffer2D> AnimatedTextureBuffer2D::create
(
    std::vector<TextureBuffer2D*>& frames,
    bool gainFrameOwnership
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<AnimatedTextureBuffer2D>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLAnimatedTextureBuffer2D>
                (
                    frames, gainFrameOwnership
                );
        }
    }
    catch(...){}
    return nullUniquePtr<AnimatedTextureBuffer2D>();
}

uint64_t AnimatedTextureBuffer2D::maxMemoryFootprint() const
{
    return  (uint64_t)frames_.size()*
            (uint64_t)width_*
            (uint64_t)height_*
            (uint64_t)internalFormatToBytes.at(internalFormat_)*
            (4./3.); // Account for mipmaps (in 2D, the mipmaps are 1./3. of
                     // of the total base texture size as it converges like
                     // 1/(2^n)^2)
}

TextureBuffer2D* AnimatedTextureBuffer2D::nextFrame() 
{
    if (frames_.size() == 0)
        return nullptr;
    ++frameIndex_;
    time_ += frameDuration_;
    frameIndex_ %= frames_.size();
    frame_ = frames_[frameIndex_];
    return frame_;
}

TextureBuffer2D* AnimatedTextureBuffer2D::previousFrame() 
{
    if (frames_.size() == 0)
        return nullptr;
    --frameIndex_;
    time_ -= frameDuration_;
    // It's an unsigned int so it overflows when <0
    if (frameIndex_ > frames_.size())
        frameIndex_ = frames_.size()-1;
    frame_ = frames_[frameIndex_];
    return frame_;
}

int AnimatedTextureBuffer2D::frameId() const
{
    if (frame_ == nullptr)
        return -1;
    return frame_->id();
}

void AnimatedTextureBuffer2D::setFrameIndex(uint32_t index)
{
    if (frames_.size() == 0)
        return;
    frameIndex_ = index % frames_.size();
    time_ = frameIndex_*frameDuration_;
    frame_ = frames_[frameIndex_];
}

void AnimatedTextureBuffer2D::setTime(float time)
{
    if (frames_.size() == 0)
        return;
    frameIndex_ = (int)std::floor(time/frameDuration_) % frames_.size();
    time_ = time;
    float duration = frames_.size()*frameDuration_;
    if (time_ > duration)
        time_ -= std::floor(time/duration)*duration;
    frame_ = frames_[frameIndex_];
}

void AnimatedTextureBuffer2D::setFrameDuration(float dt)
{
    if (dt == 0)
        return;
    float d = dt/frameDuration_;
    frameDuration_*=d;
    time_*=d;
}

void AnimatedTextureBuffer2D::setFps(float fps)
{
    setFrameDuration(1.f/fps);
}

void AnimatedTextureBuffer2D::setDuration(float t)
{
    if (frames_.size() == 0)
        return;
    int frameIndex0 = frameIndex_;
    setFrameDuration(t/frames_.size());
    setFrameIndex(frameIndex0);
}

void AnimatedTextureBuffer2D::advanceTime(float dt)
{
    setTime(time_+dt);
}

// CubeMap -------------------------------------------------------------------//

UniquePtr<CubeMapBuffer> CubeMapBuffer::create
(
    std::string filepaths[6], 
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<CubeMapBuffer>();
    UniquePtr<CubeMapBuffer> buffer;
    const unsigned char* faceData[6] = 
        {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    int i = 0;
    try
    {
        stbi_set_flip_vertically_on_load(false);
        int width0 = 0, height0 = 0, nChannels0 = 0;
        for (i=0; i<6; i++)
        {
            int width = 0, height = 0, nChannels = 0;
            faceData[i] = stbi_load
            (
                filepaths[i].c_str(), 
                &width, 
                &height, 
                &nChannels, 
                TextureBuffer::nChannels(internalFormat)
            );
            if (i == 0)
            {
                width0 = width;
                height0 = height;
                ENFORCE_CHANNEL_FORMAT_CONSISTENCY
                nChannels0 = nChannels;
            }
            else if 
            (
                width != width0 || 
                height != height0 || 
                nChannels != nChannels0 ||
                !faceData[i]
            )
                throw std::runtime_error
                (
                    "CubeMapBuffer::create - Invalid file data"
                );
        }
        buffer = CubeMapBuffer::create
        (
            faceData,
            width0,
            height0,
            internalFormat
        );
        for (int j=0; j<6; j++)
            stbi_image_free((void*)faceData[j]);
    }
    catch(...)
    {
        for (int j=0; j<i; j++)
            stbi_image_free((void*)faceData[j]);
    }
    return buffer;
}

UniquePtr<CubeMapBuffer> CubeMapBuffer::create
(
    const unsigned char* fileData[6], 
    uint32_t size,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<CubeMapBuffer>();
    UniquePtr<CubeMapBuffer> buffer;
    const unsigned char* faceData[6] = 
        {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    int i = 0;
    try
    {
        stbi_set_flip_vertically_on_load(false);
        int width0 = 0, height0 = 0, nChannels0 = 0;
        for (i=0; i<6; i++)
        {
            int width = 0, height = 0, nChannels = 0;
            faceData[i] = stbi_load_from_memory
            (
                fileData[i], 
                size,
                &width, 
                &height, 
                &nChannels, 
                TextureBuffer::nChannels(internalFormat)
            );
            ENFORCE_CHANNEL_FORMAT_CONSISTENCY
            if (i == 0)
            {
                width0 = width;
                height0 = height;
                nChannels0 = nChannels;
            }
            else if 
            (
                width != width0 || 
                height != height0 || 
                nChannels != nChannels0 || 
                !fileData[i] ||
                !faceData[i]
            )
                throw std::runtime_error
                (
                    "CubeMapBuffer::create - Invalid data"
                );
        }
        buffer = CubeMapBuffer::create
        (
            faceData,
            width0,
            height0,
            internalFormat
        );
        for (int j=0; j<6; j++)
            stbi_image_free((void*)faceData[j]);
    }
    catch(...)
    {
        for (int j=0; j<i; j++)
            stbi_image_free((void*)faceData[j]);
    }
    return buffer;
}

UniquePtr<CubeMapBuffer> CubeMapBuffer::create
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<CubeMapBuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLCubeMapBuffer>
                (
                    faceData, 
                    width, 
                    height,
                    internalFormat
                );
        }
    }
    catch(...){}
    return nullUniquePtr<CubeMapBuffer>();
}

bool CubeMapBuffer::validFace(const TextureBuffer2D* face)
{
    auto isPowerOfTwo = [](uint32_t x)->bool{return(x!=0)&&((x&(x-1))==0);};
    if (face->width() != face->height())
        return false;
    if (!isPowerOfTwo(face->width()) || !isPowerOfTwo(face->height()))
        return false;
    return true;
}

bool CubeMapBuffer::validFaces(const TextureBuffer2D* faces[6])
{
    auto width = faces[0]->width();
    auto height = faces[0]->height();
    if (width != height)
        return false;
    for (uint32_t i=0; i<6; i++)
    {
        if (faces[i]->width() != width || faces[i]->height() != height)
            return false;
        if (!validFace(faces[i]))
            return false;
    }
    return true;
}

uint64_t CubeMapBuffer::maxMemoryFootprint() const
{
    return  (uint64_t)width_*
            (uint64_t)height_*
            (uint64_t)internalFormatToBytes.at(internalFormat_)*
            (8l); // 6 faces, each 4/3 of the base size to account for 
                  // mipmaps, so 6*4/3 = 8
}

// Texture3D -----------------------------------------------------------------//

UniquePtr<TextureBuffer3D> TextureBuffer3D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    InternalFormat internalFormat
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<TextureBuffer3D>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLTextureBuffer3D>
                (
                    data, 
                    width, 
                    height,
                    depth,
                    internalFormat
                );
        }
    }
    catch(...){}
    return nullUniquePtr<TextureBuffer3D>();
}

uint32_t TextureBuffer3D::maxSideSize()
{
    if (!GlobalPtr<Window>::valid())
        return 0;
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return OpenGLTextureBuffer3D::maxSideSize();
        }
    }
    catch(...){}
    return 0;
}

uint64_t TextureBuffer3D::maxMemoryFootprint() const
{
    return  (uint64_t)width_*
            (uint64_t)height_*
            (uint64_t)depth_*
            (uint64_t)internalFormatToBytes.at(internalFormat_)*
            (8./7.); // Account for mipmaps (in 3D, the mipmaps are 1./7. of
                     // of the total base texture size as it converges like
                     // 1/(2^n)^3)
}

// Framebuffer ---------------------------------------------------------------//

Framebuffer* Framebuffer::activeOne_ = nullptr;

UniquePtr<Framebuffer> Framebuffer::create
(
    uint32_t width, 
    uint32_t height,
    TextureBuffer::InternalFormat format
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<Framebuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLFramebuffer>(width, height, format);
        }
    }
    catch(...){}
    return nullUniquePtr<Framebuffer>();
}

// Vertex Buffer layout ------------------------------------------------------//

VertexBufferLayout::VertexBufferLayout
(
    const std::initializer_list<VertexBufferLayout::Element>& elements
) :
stride_(0),
elements_(elements)
{
    int location = 0;
    for (Element& e : elements_)
    {
        e.offset = stride_;
        e.location = location;
        stride_ += e.attribute.size;
        location += 1;
    }
}

// Vertex Buffer -------------------------------------------------------------//

UniquePtr<VertexBuffer> VertexBuffer::create(float* vertices, uint32_t size)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<VertexBuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLVertexBuffer>(vertices, size);
        }
    }
    catch(...){}
    return nullUniquePtr<VertexBuffer>();
}

VertexBuffer::~VertexBuffer()
{
    //if (layout_ != nullptr)
    //    delete layout_;
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindVertexBuffer(this);
    }
}

void VertexBuffer::setLayout
(
    const std::initializer_list<VertexBufferLayout::Element>& elements
)
{
    layout_ = makeUnique<VertexBufferLayout>(elements);
    setLayout(*layout_);
}

void VertexBuffer::setLayout()
{
    if (layout_.valid())
        setLayout(*layout_);
}

// Index buffer --------------------------------------------------------------//

UniquePtr<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t size)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<IndexBuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLIndexBuffer>(indices, size);
        }
    }
    catch(...){}
    return nullUniquePtr<IndexBuffer>();
}

IndexBuffer::~IndexBuffer()
{
    for (VertexArray* va : vertexArrays_)
    {
        va->unbindIndexBuffer();
    }
}

// Vertex array --------------------------------------------------------------//

VertexArray* VertexArray::activeOne_ = nullptr;

UniquePtr<VertexArray> VertexArray::create()
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<VertexArray>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLVertexArray>();
        }
    }
    catch(...){}
    return nullUniquePtr<VertexArray>();
}

// Uniform Buffer Object -----------------------------------------------------//

UniquePtr<UniformBuffer> UniformBuffer::create(uint32_t size)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<UniformBuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLUniformBuffer>(size);
        }
    }
    catch(...){}
    return nullUniquePtr<UniformBuffer>();
}

// Uniform Buffer Object  2 --------------------------------------------------//

DynamicUniformBuffer::DynamicUniformBuffer
(
    uint32_t maxSize, 
    const std::string& name
) :
id_(0), 
size_(0), 
maxSize_(maxSize), 
bindingPoint_(-1), 
name_(name)
{
    rawBuffer_ = new unsigned char[maxSize];
};

UniquePtr<DynamicUniformBuffer> DynamicUniformBuffer::create
(
    uint32_t size, 
    const std::string& name
)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<DynamicUniformBuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLDynamicUniformBuffer>(size, name);
                break;
        }
    }
    catch(...){}
    return nullUniquePtr<DynamicUniformBuffer>();
}

DynamicUniformBuffer::~DynamicUniformBuffer()
{
    if (rawBuffer_ != nullptr)
        delete[] rawBuffer_;
    for (unsigned int i=0; i<uniformWrappers_.size(); i++)
    {
        delete uniformWrappers_[i];
    }
    uniformWrappers_.resize(0);
}

uint32_t DynamicUniformBuffer::sizeOf(const Uniform* uniform) const
{
    return 
        uniform == nullptr ?
        0.0 :
        uniform->isValueArray() ? 
        arrayElementSizeOf(uniform) * uniform->valueArraySize() :
        typeSizeOf(uniform);
}

bool DynamicUniformBuffer::addUniform(WeakPtr<Uniform> uniform)
{
    auto size = sizeOf(uniform.get());
    if 
    (
        !uniform.valid() || 
        uniformWrappersMap_.find(uniform.get()) != uniformWrappersMap_.end() ||
        size == 0 ||
        size_ + size > maxSize_
    )
        return false;
    auto uw = new UniformWrapper{uniform};
    if (!uniformWrappers_.empty()) // Set previous/next
    {
        uw->previous = uniformWrappers_.back();
        uniformWrappers_.back()->next = uw;
    }
    if (!uniform->name.empty())
    {
        uw->markedForSubmission = true;
        nUniformsMarkedForSubmission_++;
    }
    if (uniform->isValueArray())
    {
        uw->arraySubmissionIndexStart = 0u;
        uw->arraySubmissionIndexEnd = uniform->valueArraySize()-1;
    }   
    uniformWrappers_.emplace_back(uw);
    uniformWrappersMap_.insert({uniform.get(), uw});
    recalculateUniformSizesAndOffsets();
    return true;
}

bool DynamicUniformBuffer::removeUniform(const Uniform* uniform)
{
    auto it = std::find_if
    (
        uniformWrappers_.begin(),
        uniformWrappers_.end(),
        [&uniform](const auto& uw){return uw->uniform==uniform;}
    );
    if (it == uniformWrappers_.end())
        return false;
    auto uw = *it;
    if (uw->markedForSubmission)
        nUniformsMarkedForSubmission_--;
    if (uw->next != nullptr)
        uw->next->previous = uw->previous;
    if (uw->previous != nullptr)
        uw->previous->next = uw->next;
    delete uw;
    uniformWrappers_.erase(it);
    uniformWrappersMap_.erase(uniform);
    recalculateUniformSizesAndOffsets();
    return true;
}

void DynamicUniformBuffer::recalculateUniformSizesAndOffsets()
{
    uint32_t offset = 0;
    // First uniform on which changes in alignment, offsets begin
    UniformWrapper* uw0 = nullptr;
    for (auto* uw : uniformWrappers_)
    {
        auto alignment = alignmentOf(uw->uniform.get());
        if (offset % alignment > 0)
            offset += alignment - (offset % alignment);
        if (uw0 == nullptr && uw->offset != offset)
            uw0 = uw;
        uw->offset = offset;
        
        auto size = sizeOf(uw->uniform.get());
        if (uw0 == nullptr && uw->size != size)
            uw0 = uw;
        uw->size = size;
        
        auto typeSize = typeSizeOf(uw->uniform.get());
        if (uw0 == nullptr && uw->typeSize != typeSize)
            uw0 = uw;
        uw->typeSize = typeSizeOf(uw->uniform.get());
        
        if (uw->isUniformArray())
        {
            auto arrayElementSize = arrayElementSizeOf(uw->uniform.get());
            if (uw0 == nullptr && uw->arrayElementSize != arrayElementSize)
                uw0 = uw;
            uw->arrayElementSize = arrayElementSize;
        }
        else
            uw->arrayElementSize = 0u;
        
        offset += uw->size;
    }
    size_ = offset;
    // Minor issue: the maxSize_ can be exceeded even without adding new
    // uniforms: it is sufficient to change the types of uniforms already
    // in the buffer. How to handle this case? For the time being, whatever

    // Reupload the data of all uniforms subsequent to uw0
    while (uw0 != nullptr)
    {
        markUniformForSubmission(uw0->uniform.get());
        uw0 = uw0->next;
    }
    submitUniforms();
}

bool DynamicUniformBuffer::markUniformForSubmission
(
    const Uniform* uniform,
    uint32_t indexStart,
    uint32_t indexEnd
)
{
    auto it = uniformWrappersMap_.find(uniform);
    if (it == uniformWrappersMap_.end())
        return false;
    auto* uw = it->second;
    if (!uw->markedForSubmission)
    {
        nUniformsMarkedForSubmission_++;
        uw->markedForSubmission = true;
        if (uw->uniform.valid() && uniform->isValueArray())
        {
            uw->arraySubmissionIndexStart = indexStart;
            uw->arraySubmissionIndexEnd = std::max(indexEnd, indexStart);
        }
        return true;
    }
    return false;
}

bool DynamicUniformBuffer::markUniformForSubmission
(
    const Uniform* uniform
)
{
    return markUniformForSubmission
    (
        uniform, 
        0u, 
        uniform->isValueArray() ? uniform->valueArraySize()-1u : 0u
    );
}

bool DynamicUniformBuffer::markArrayUniformRangeForSubmission
(
    const Uniform* uniform,
    uint32_t arrayIndexStart,
    uint32_t arrayIndexEnd
)
{
    return markUniformForSubmission
    (
        uniform, 
        arrayIndexStart, 
        arrayIndexEnd
    );
}

bool DynamicUniformBuffer::markContiguousUniformsForSubmission
(
    const Uniform* uniform0, 
    const Uniform* uniform1
)
{
    if (uniformWrappers_.empty())
        return false;

    auto it0 = uniformWrappersMap_.find(uniform0);
    auto uw0 = it0 == uniformWrappersMap_.end() ? 
        uniformWrappers_.front() : 
        it0->second;
    auto it1 = uniformWrappersMap_.find(uniform1);
    auto uw1 = it1 == uniformWrappersMap_.end() ? 
        uniformWrappers_.back() : 
        it1->second;

    auto uw = uw0;
    while (uw != uw1->next)
    {
        if (uw->markedForSubmission)
        {
            uw = uw->next;
            continue;
        }
        nUniformsMarkedForSubmission_++;
        uw->markedForSubmission = true;
        if (uw->uniform.valid() && uw->uniform->isValueArray())
        {
            uw->arraySubmissionIndexStart = 0;
            uw->arraySubmissionIndexEnd = uw->uniform->valueArraySize()-1;
        }
        uw = uw->next;
    }
    return true;
}

void DynamicUniformBuffer::submitUniforms(bool forceSubmitAllUniforms)
{
    bool someUniformsToBeDeleted = false;
    if 
    (
        uniformWrappers_.empty() || 
        (
            !forceSubmitAllUniforms &&
            nUniformsMarkedForSubmission_ == 0u
        )
    )
        return;
    if (nUniformsMarkedForSubmission_ == uniformWrappers_.size())
        forceSubmitAllUniforms = true;
    if (forceSubmitAllUniforms) // Simple case first
    {
        for (auto& uw : uniformWrappers_)
        {
            if (!uw->uniform.valid())
            {
                uw->markedForDeletion = true;
                someUniformsToBeDeleted = true;
                continue;
            }
            if (uw->uniform->isValueArray())
            {
                const unsigned char* src;
                if (uw->uniform->type() != Uniform::Type::Bool)
                    src = reinterpret_cast<const unsigned char*>
                    (
                        uw->uniform->getNativeValue()
                    );
                else
                {
                    auto boolArray = uw->uniform->getConstValuePtr<bool>();
                    auto intArray = new int[uw->uniform->valueArraySize()];
                    for (unsigned int i=0; i<uw->uniform->valueArraySize(); i++)
                    {
                        intArray[i] = static_cast<int>(boolArray[i]);
                    }
                    src = reinterpret_cast<const unsigned char*>(intArray);
                }
                for (unsigned int i=0; i<uw->uniform->valueArraySize(); i++)
                {
                    std::memcpy
                    (
                        rawBuffer_ + uw->offset + i*uw->arrayElementSize, 
                        src + i*uw->typeSize, 
                        uw->typeSize
                    );
                }
                if (uw->uniform->type() == Uniform::Type::Bool)
                    delete[] src;
            }
            else
            {
                const unsigned char* src = 
                    uw->uniform->type() != Uniform::Type::Bool ?
                    reinterpret_cast<const unsigned char*>
                    (
                        uw->uniform->getNativeValue()
                    ) :
                    reinterpret_cast<const unsigned char*>
                    (
                        new int(uw->uniform->getValue<bool>())
                    );
                std::memcpy
                (
                    rawBuffer_ + uw->offset, 
                    src, 
                    uw->size
                );
                if (uw->uniform->type() == Uniform::Type::Bool)
                    delete src;
            }
            uw->markedForSubmission = false;
        }
        submitData(rawBuffer_, size_, 0u);
        nUniformsMarkedForSubmission_ = 0u;
    }
    else
    {
        // Only update data of uniforms marked for submission. The most
        // efficient way is to chop the data into contiguous blocks of
        // uniforms marked for submission, and upload said blocks in
        // one go each. This is more efficient than submitting every uniform
        // as a separate block
        UniformWrapper* uw = uniformWrappers_[0];
        UniformWrapper* uw0 = nullptr; // Block start
        UniformWrapper* uw1 = nullptr; // Block end
        do
        {
            if (!uw->uniform.valid())
            {
                uw->markedForDeletion = true;
                someUniformsToBeDeleted = true;
                uw = uw->next;
                continue;
            }
            if 
            (
                uw->markedForSubmission && 
                uw->uniform->getNativeValue() != nullptr
            )
            {
                if (uw0 == nullptr)
                    uw0 = uw;
                uw1 = uw;
                uw->markedForSubmission = false;
                nUniformsMarkedForSubmission_--;
            }
            if 
            (
                uw0 != nullptr && 
                uw1 != nullptr &&
                (
                    uw1->next == nullptr || 
                    uw1->next->markedForSubmission == false ||
                    uw1->next->uniform->isValueArray() ||
                    uw0->uniform->isValueArray()
                )
            )
            {
                // If the uniform is an array, treat its range as a separate
                // block altogether for simplicity (it can only be merged with
                // adjacent blocks if the initial or final or whole range are
                // marked for submission, which might not generally be the case)
                if (uw0->uniform->isValueArray())
                    submitArrayUniformRangeNoCheck
                    (
                        uw0, 
                        uw0->arraySubmissionIndexStart,
                        uw0->arraySubmissionIndexEnd
                    );
                else
                {
                    uint32_t blockSize = 
                        uw1->size + uw1->offset - uw0->offset;
                    uw = uw0;
                    while (true)
                    {
                        const unsigned char* src = 
                            uw->uniform->type() != Uniform::Type::Bool ?
                            reinterpret_cast<const unsigned char*>
                            (
                                uw->uniform->getNativeValue()
                            ) :
                            reinterpret_cast<const unsigned char*>
                            (
                                new int(uw->uniform->getValue<bool>())
                            );
                        std::memcpy
                        (
                            rawBuffer_ + uw->offset - uw0->offset, 
                            src, 
                            uw->size
                        );
                        if (uw->uniform->type() == Uniform::Type::Bool)
                            delete src;
                        if (uw == uw1)
                            break;
                        uw = uw->next;
                    };
                    submitData(rawBuffer_, blockSize, uw0->offset);
                }
                uw0 = nullptr;
                uw1 = nullptr;
            }
            uw = uw->next;
        }
        while (uw != nullptr && nUniformsMarkedForSubmission_ > 0);
    }
    if (someUniformsToBeDeleted)
    {
        for (uint32_t i=0; i<uniformWrappers_.size(); i++)
        {
            auto* uw = uniformWrappers_[i];
            if (uw->markedForDeletion)
            {
                removeUniform(uw->uniform.get());
                i--;
            }
        }
    }
}

bool DynamicUniformBuffer::submitArrayUniformRangeNoCheck
(
    UniformWrapper* uw,
    uint32_t indexStart,
    uint32_t indexEnd
)
{
    indexEnd = std::max(indexStart, indexEnd);
    uint32_t nElements = indexEnd-indexStart+1u;
    uint32_t blockSize = nElements*uw->arrayElementSize;
    if (blockSize == 0)
        return false;
    if (!uw->uniform.valid())
    {
        removeUniform(uw->uniform.get());
        return false;
    }
    const unsigned char* src;
    if (uw->uniform->type() != Uniform::Type::Bool)
        src = reinterpret_cast<const unsigned char*>
        (
            uw->uniform->getNativeValue()
        ) + indexStart*uw->typeSize;
    else
    {
        auto boolArray = uw->uniform->getConstValuePtr<bool>();
        auto intArray = new int[nElements];
        for (unsigned int i = 0; i < nElements; i++) 
        {
            intArray[i] = static_cast<int>(boolArray[i+indexStart]);
        }
        src = reinterpret_cast<const unsigned char*>(intArray);
    }
    for (unsigned int i=0; i<nElements; i++)
    {
        std::memcpy
        (
            rawBuffer_ + i*uw->arrayElementSize, 
            src + i*uw->typeSize, 
            uw->typeSize
        );
    }
    submitData
    (
        rawBuffer_, 
        blockSize, 
        uw->offset + indexStart*uw->arrayElementSize
    );
    if (uw->uniform->type() == Uniform::Type::Bool)
        delete[] src;
    uw->arraySubmissionIndexStart = 0u;
    uw->arraySubmissionIndexEnd = uw->uniform->valueArraySize()-1u;
    return true;
}

bool DynamicUniformBuffer::submitUniform
(
    const Uniform* uniform,
    uint32_t indexStart,
    uint32_t indexEnd
)
{
    auto it = uniformWrappersMap_.find(uniform);
    if (it == uniformWrappersMap_.end())
        return false;
    auto* uw = it->second;
    if (!uw->uniform.valid())
    {
        removeUniform(uw->uniform.get());
        return false;
    }
    if (uw->uniform->isValueArray())
        submitArrayUniformRangeNoCheck(uw, indexStart, indexEnd);
    else
    {
        const void* src = 
            uw->uniform->type() != Uniform::Type::Bool ?
            uw->uniform->getNativeValue() :
            static_cast<void*>
            (
                new int(uw->uniform->getValue<bool>())
            );
        submitData
        (
            src,
            uw->size,
            uw->offset
        );
        if (uw->uniform->type() == Uniform::Type::Bool)
            delete static_cast<const bool*>(src);
        if (uw->markedForSubmission)
        {
            uw->markedForSubmission = false;
            nUniformsMarkedForSubmission_--;
        };
    }
    return true;
}

bool DynamicUniformBuffer::submitUniform
(
    const Uniform* uniform
)
{
    return submitUniform
    (
        uniform, 
        0, 
        uniform->isValueArray() ? uniform->valueArraySize()-1u : 0u
    );
}

bool DynamicUniformBuffer::submitArrayUniformRange
(
    const Uniform* uniform,
    uint32_t indexStart,
    uint32_t indexEnd
)
{
    return submitUniform
    (
        uniform, 
        indexStart, 
        indexEnd
    );
}

// Shader Storage Buffer Object ----------------------------------------------//

UniquePtr<ShaderStorageBuffer> ShaderStorageBuffer::create(uint32_t size)
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<ShaderStorageBuffer>();
    try
    {
        switch(Window::instance()->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return makeUnique<OpenGLShaderStorageBuffer>(size);
        }
    }
    catch(...){}
    return nullUniquePtr<ShaderStorageBuffer>();
}

}