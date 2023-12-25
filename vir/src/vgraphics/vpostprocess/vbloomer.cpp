#include "vpch.h"
#include "vgraphics/vpostprocess/vbloomer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglbloomer.h"

namespace vir
{
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
}