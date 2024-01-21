#ifndef V_INITIALIZATION_H
#define V_INITIALIZATION_H

#include <string>

namespace vir
{

enum class PlatformType
{
    None,
    GLFWOpenGL
};

extern PlatformType platform;

// Initialized the global Window, Event::Broadcaster, InputState and Renderer
// objects based on the provided platform type (only one currently supported,
// i.e., GLFWOpenGL). Note that Window is internally responsible for 
// initializing the global Time and GraphicsContext objects.
void initialize
(
    PlatformType p,
    uint32_t width=1200, 
    uint32_t height=600, 
    std::string windowName="Vir.exe",
    bool windowResizable=true,
    bool initializeImGuiRenderer=true
);

}

#endif