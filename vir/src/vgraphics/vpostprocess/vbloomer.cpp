#include "vpch.h"
#include "vgraphics/vpostprocess/vbloomer.h"
#include "vgraphics/vpostprocess/vopengl/vopenglbloomer.h"

namespace vir
{

std::unordered_map<Bloomer::Settings::ToneMap, std::string> 
    Bloomer::toneMapToName = 
    {
        {Settings::ToneMap::ACES, "ACES"},
        {Settings::ToneMap::ReinhardExtended, "Reinhard (ext.)"},
        {Settings::ToneMap::Reinhard, "Reinhard"},
        {Settings::ToneMap::None, "None"}
    };

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