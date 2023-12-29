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
            ReinhardExtended = 2,
            ACES = 3
        };
        unsigned int mipDepth = 16;
        float intensity = 1.f;
        float threshold = .8f;
        float knee = .1f;
        float coreDimming = 0.f;
        ToneMap toneMap = ToneMap::ACES;
        float reinhardExposure = 2.f;
        float reinhardWhitePoint = 1.f;
    };

    static const std::unordered_map<Settings::ToneMap, std::string>
        toneMapToName;

    static constexpr const unsigned int maxMipDepth = 16;

protected:

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

    //
    virtual void bloom
    (
        const Framebuffer* input,
        Settings& settings
    ) = 0;
};

}

#endif