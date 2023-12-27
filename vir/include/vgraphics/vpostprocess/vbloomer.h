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

    enum class ToneMap
    {
        None,
        Reinhard,
        ReinhardExtended,
        ACES
    };

    struct Settings
    {
        float intensity = 1.f;
        float threshold = .8f;
        float knee = .1f;
        float radius = 1e-4f;
        ToneMap toneMap = ToneMap::ACES;

        int mip = 10;
    };

protected:

    // Cached settings
    Settings settings_;

    // Protected constructor as any instances of Quantizer are meant to be
    // created via the static create function
    Bloomer() :
        PostProcess(Type::Bloom),
        settings_({}){}

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
        Settings settings
    ) = 0;

};

}

#endif