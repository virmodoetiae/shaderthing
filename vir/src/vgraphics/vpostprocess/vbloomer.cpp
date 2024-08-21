#include "vpch.h"
#include "vgraphics/vpostprocess/vbloomer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglbloomer.h"

namespace vir
{

const std::unordered_map<Bloomer::Settings::ToneMap, std::string> 
    Bloomer::toneMapToName = 
    {
        {Settings::ToneMap::ACES, "ACES"},
        {Settings::ToneMap::Reinhard, "Reinhard"},
        //{Settings::ToneMap::Radman, "Radman"},
        {Settings::ToneMap::None, "None"}
    };

Bloomer* Bloomer::create()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLBloomer();
    }
    return nullptr;
}

// Just a fancy integer log2(x)-1 implementation
unsigned int Bloomer::maxMipLevel
(
    const Framebuffer* framebuffer
)
{
    unsigned int size = 
        std::max(framebuffer->width(), framebuffer->height())-1u;
#if defined(__GNUC__) || defined(__clang__)
    #include <cstdint>
    return 
        std::min(31u-__builtin_clz(size), maxMipDepth_);
#elif defined(_MSC_VER)
    #include <intrin.h>
    unsigned long index;
    if (_BitScanReverse(&index, size))
        return std::min(31u - (unsigned int)index, maxMipDepth_);
    return maxMipDepth_-1u;
#else
    unsigned int level = 0;
    while (x >>= 1) 
    {
        ++level;
    }
    return std::min(level, maxMipDepth_);
#endif
}

}