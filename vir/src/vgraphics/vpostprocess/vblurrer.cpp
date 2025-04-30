#include "vpch.h"
#include "vgraphics/vpostprocess/vblurrer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglblurrer.h"

namespace vir
{

Blurrer* Blurrer::create()
{
    if (!GlobalPtr<Window>::valid())
        return nullptr;
    switch(Window::instance()->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLBlurrer();
    }
    return nullptr;
}
}