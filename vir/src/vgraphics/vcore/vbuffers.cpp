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

TextureBuffer2D* TextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLTextureBuffer2D
                (
                    data, 
                    width, 
                    height,
                    internalFormat
                );
        }
    }
    catch(...){}
    return nullptr;
}

TextureBuffer2D* TextureBuffer2D::create
(
    const unsigned char* fileData, 
    uint32_t size,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    unsigned char* data = nullptr;
    TextureBuffer2D* buffer = nullptr;
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
    
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                buffer =  new OpenGLTextureBuffer2D
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

TextureBuffer2D* TextureBuffer2D::create
(
    std::string filepath, 
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    unsigned char* data = nullptr;
    TextureBuffer2D* buffer = nullptr;
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
    
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                buffer =  new OpenGLTextureBuffer2D
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

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
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
        return nullptr;
    }
    auto buffer = AnimatedTextureBuffer2D::create
    (
        fileData,
        size,
        internalFormat
    );
    delete[] fileData;
    return buffer;
}

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
(
    const unsigned char* fileData, 
    uint32_t size,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    unsigned char* data = nullptr;
    int* delays = nullptr;
    AnimatedTextureBuffer2D* buffer = nullptr;
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
        
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                buffer = new OpenGLAnimatedTextureBuffer2D
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

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t nFrames,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLAnimatedTextureBuffer2D
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
    return nullptr;
}

AnimatedTextureBuffer2D* AnimatedTextureBuffer2D::create
(
    std::vector<TextureBuffer2D*>& frames,
    bool gainFrameOwnership
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLAnimatedTextureBuffer2D
                (
                    frames, gainFrameOwnership
                );
        }
    }
    catch(...){}
    return nullptr;
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

CubeMapBuffer* CubeMapBuffer::create
(
    std::string filepaths[6], 
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    CubeMapBuffer* buffer = nullptr;
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

CubeMapBuffer* CubeMapBuffer::create
(
    const unsigned char* fileData[6], 
    uint32_t size,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    CubeMapBuffer* buffer = nullptr;
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

CubeMapBuffer* CubeMapBuffer::create
(
    const unsigned char* faceData[6], 
    uint32_t width,
    uint32_t height,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLCubeMapBuffer
                (
                    faceData, 
                    width, 
                    height,
                    internalFormat
                );
        }
    }
    catch(...){}
    return nullptr;
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

// Texture3D -----------------------------------------------------------------//

TextureBuffer3D* TextureBuffer3D::create
(
    const unsigned char* data, 
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    InternalFormat internalFormat
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLTextureBuffer3D
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
    return nullptr;
}

uint32_t TextureBuffer3D::maxSize()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return 0;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return OpenGLTextureBuffer3D::maxSize();
        }
    }
    catch(...){}
    return 0;
}

// Framebuffer ---------------------------------------------------------------//

Framebuffer* Framebuffer::activeOne_ = nullptr;

Framebuffer* Framebuffer::create
(
    uint32_t width, 
    uint32_t height,
    TextureBuffer::InternalFormat format
)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLFramebuffer(width, height, format);
        }
    }
    catch(...){}
    return nullptr;
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
        stride_ += e.variable.size;
        location += 1;
    }
}

// Vertex Buffer -------------------------------------------------------------//

VertexBuffer* VertexBuffer::create(float* vertices, uint32_t size)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLVertexBuffer(vertices, size);
        }
    }
    catch(...){}
    return nullptr;
}

VertexBuffer::~VertexBuffer()
{
    if (layout_ != nullptr)
        delete layout_;
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
    layout_ = new VertexBufferLayout(elements);
    setLayout(*layout_);
}

void VertexBuffer::setLayout()
{
    if (layout_ != nullptr)
        setLayout(*layout_);
}

// Index buffer --------------------------------------------------------------//

IndexBuffer* IndexBuffer::create(uint32_t* indices, uint32_t size)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLIndexBuffer(indices, size);
        }
    }
    catch(...){}
    return nullptr;
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

VertexArray* VertexArray::create()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLVertexArray();
        }
    }
    catch(...){}
    return nullptr;
}

// Uniform Buffer Object -----------------------------------------------------//

UniformBuffer* UniformBuffer::create(uint32_t size)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLUniformBuffer(size);
        }
    }
    catch(...){}
    return nullptr;
}

// Shader Storage Buffer Object ----------------------------------------------//

ShaderStorageBuffer* ShaderStorageBuffer::create(uint32_t size)
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    try
    {
        switch(window->context()->type())
        {
            case (GraphicsContext::Type::OpenGL) :
                return new OpenGLShaderStorageBuffer(size);
        }
    }
    catch(...){}
    return nullptr;
}

}