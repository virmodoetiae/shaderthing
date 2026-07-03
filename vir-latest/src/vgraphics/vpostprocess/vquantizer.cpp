#include "vpch.h"
#include "vgraphics/vpostprocess/vquantizer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglquantizer.h"

namespace vir
{

typedef Quantizer::Settings::IndexMode IndexMode;
typedef Quantizer::Settings::DitherMode DitherMode;

std::unordered_map<IndexMode, std::string> 
    Quantizer::Settings::indexModeToName = 
    {
        {IndexMode::Delta, "Delta indexing"},
        {IndexMode::Alpha, "Alpha indexing"},
        {IndexMode::Default, "Default indexing"}
        
    };

std::unordered_map<DitherMode, std::string> 
    Quantizer::Settings::ditherModeToName = 
    {
        {DitherMode::Order4, "4x4 Kernel"},
        {DitherMode::Order2, "2x2 Kernel"},
        {DitherMode::None, "None"}
    };

UniquePtr<Quantizer> Quantizer::create()
{
    if (!GlobalPtr<Window>::valid())
        return nullUniquePtr<Quantizer>();
    switch(Window::instance()->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return  makeUnique<OpenGLQuantizer>();
    }
    return nullUniquePtr<Quantizer>();
}

}