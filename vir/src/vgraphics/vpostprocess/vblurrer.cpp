#include "vpch.h"
#include "vgraphics/vpostprocess/vblurrer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglblurrer.h"

namespace vir
{

Blurrer* Blurrer::create()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLBlurrer();
    }
    return nullptr;
}
}