#ifndef V_BLOOMER_H
#define V_BLOOMER_H

#include "vgraphics/vpostprocess/vpostprocess.h"

namespace vir
{

class Framebuffer;
class TextureBuffer2D;

class Bloomer : public PostProcess
{
public:

    struct Settings
    {
        enum class ToneMap
        {
            None = 0,
            Reinhard = 1,
            ACES = 2
        };
        unsigned int mipDepth = 16;
        float intensity = 1.f;
        float threshold = .8f;
        float knee = .1f;
        float haze = 0.f;
        ToneMap toneMap = ToneMap::ACES;
        float reinhardWhitePoint = 1.f;
    };

    static const std::unordered_map<Settings::ToneMap, std::string>
        toneMapToName;

protected:

    static constexpr const unsigned int maxMipDepth_ = 16;
    static constexpr const unsigned int minSideResolution_ = 2; // Pixels

    // Protected constructor as any instances of Quantizer are meant to be
    // created via the static create function
    Bloomer() : PostProcess(Type::Bloom){}

    // Delete copy-construction & copy-assignment ops
    Bloomer(const Bloomer&) = delete;
    Bloomer& operator= (const Bloomer&) = delete;

public:

    // Create a Quantizer-type object
    static Bloomer* create();

    // Destructor
    virtual ~Bloomer(){}

    // Return the maximum possible mip level on which we can operate for the
    // provided Framebuffer
    static unsigned int maxMipLevel(const Framebuffer* framebuffer);

    //
    virtual void bloom
    (
        const Framebuffer* input,
        const Settings& settings
    ) = 0;
};

}

#endif