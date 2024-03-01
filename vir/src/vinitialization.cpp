#include "vpch.h"
#include "vinitialization.h"
#include "vwindow/vglfwopenglwindow.h"
#include "veventsystem/vglfwbroadcaster.h"

namespace vir
{
    
PlatformType platform = PlatformType::None;

void initialize(const Settings& settings)
{
    platform = settings.platform;
    Window* window = nullptr;
    switch(platform)
    {
        case PlatformType::GLFWOpenGL :
        {
            window = Window::initialize<GLFWOpenGLWindow>
            (
                settings.width, 
                settings.height, 
                settings.windowName,
                settings.enableWindowResizing
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
    window->tuneIntoEventBroadcaster(VIR_WINDOW_PRIORITY);
    InputState::initialize();
    auto renderer = Renderer::initialize();
    renderer->setDepthTesting(settings.enableDepthTesting);
    renderer->setBlending(settings.enableBlending);
    renderer->setFaceCulling(settings.enableFaceCulling);
    if (settings.initializeImGuiRenderer)
        vir::ImGuiRenderer::initialize();
}

}