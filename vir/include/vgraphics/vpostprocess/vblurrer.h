#ifndef V_BLURRER_H
#define V_BLURRER_H

#include "vgraphics/vpostprocess/vpostprocess.h"

namespace vir
{

class Framebuffer;
class TextureBuffer2D;

class Blurrer : public PostProcess
{
public:

    struct Settings
    {
        unsigned int xRadius = 5;
        unsigned int yRadius = 5;
        unsigned int subSteps = 2;
    };

protected:

    // Protected constructor as any instances of Blurrer are meant to be
    // created via the static create function
    Blurrer() : PostProcess(Type::Blur){}

    // Delete copy-construction & copy-assignment ops
    Blurrer(const Blurrer&) = delete;
    Blurrer& operator= (const Blurrer&) = delete;

public:

    //
    static Blurrer* create();

    // Destructor
    virtual ~Blurrer(){}

    //
    virtual void blur
    (
        const Framebuffer* input,
        Settings& settings
    ) = 0;
};

}

#endif