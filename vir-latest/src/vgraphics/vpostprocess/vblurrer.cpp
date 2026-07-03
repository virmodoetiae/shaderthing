#include "vpch.h"
#include "vgraphics/vpostprocess/vblurrer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglblurrer.h"

namespace vir
{

UniquePtr<Blurrer> Blurrer::create()
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<Blurrer>();
    switch(Window::instance()->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return makeUnique<OpenGLBlurrer>();
    }
    return nullUniquePtr<Blurrer>();
}
}