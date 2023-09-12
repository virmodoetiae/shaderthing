#include "vpch.h"
#include "vgraphics/vmisc/vkmeansquantizer.h"
#include "vgraphics/vmisc/vopengl/vopenglkmeansquantizer.h"

namespace vir
{

KMeansQuantizer* KMeansQuantizer::create()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLKMeansQuantizer();
    }
    return nullptr;
} 

}