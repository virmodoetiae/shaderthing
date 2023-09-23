#include "vpch.h"
#include "vinitialization.h"
#include "vwindow/vglfwopenglwindow.h"
#include "veventsystem/vglfwbroadcaster.h"

namespace vir
{
    
PlatformType platform = PlatformType::None;

void initialize
(
    PlatformType p, 
    uint32_t width, 
    uint32_t height,
    std::string windowName,
    bool windowResizable
)
{
    platform = p;
    Window* window = nullptr;
    switch(platform)
    {
        case PlatformType::GLFWOpenGL :
        {
            window = Window::initialize<GLFWOpenGLWindow>
            (
                width, 
                height, 
                windowName,
                windowResizable
            );
            Event::Broadcaster::initialize<Event::GLFWBroadcaster>();
            break;
        }
        default :
        {
            throw std::runtime_error
            (
                "Invalid platform for vir library initialization"
            );
            break;
        }
    }
    if (window == nullptr)
    {
        throw std::runtime_error("Window uninitialized");
        return;
    }
    window->tuneIn();
    InputState::initialize()->tuneIn();
    auto renderer = Renderer::initialize();
    renderer->setDepthTesting(true);
    
    std::cout << "Context: " << window->context()->name() << std::endl;
    std::cout << "Device: " << renderer->deviceName() << std::endl;
}

}