#include "shaderthing/include/oo/eventmanager.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/corelogic.h"

namespace ShaderThing
{

EventManager::EventManager(AppData& appData) : appData_(appData) 
{
    tuneIntoEventBroadcaster();
    
    // Receive MouseMotion event before the vir::InputCamera in
    // appData.sharedUniforms.shaderCamera. This is necessary to enable the
    // functionality controlled by the cameraMouseInputRequiresLMBHold flag
    setEventReceiverPriority
    (
        vir::Event::Type::MouseMotion, 
        VIR_CAMERA_PRIORITY-1
    );
    setEventReceiverPriority
    (
        vir::Event::Type::MouseButtonPress, 
        VIR_IMGUI_PRIORITY-1
    );
    setEventReceiverPriority
    (
        vir::Event::Type::MouseButtonRelease, 
        VIR_IMGUI_PRIORITY-1
    );
    setEventReceiverPriority
    (
        vir::Event::Type::KeyPress, 
        VIR_IMGUI_PRIORITY-1
    );
    setEventReceiverPriority
    (
        vir::Event::Type::KeyRelease, 
        VIR_IMGUI_PRIORITY-1
    );
}

void EventManager::onReceive(vir::Event::WindowResizeEvent& event)
{
    if (event.width == 0 || event.height == 0)
    {
        event.handled = true;
        return;
    }
    glm::ivec2 resolution{event.width, event.height};
    setWindowResolution(appData_, resolution, true);
    event.width = resolution.x;
    event.height = resolution.y;

    for (auto& layer : appData_.layers)
    {
        setLayerResolution
        (
            *layer, 
            resolution, 
            appData_.renderer.isTiledRenderingEnabled, 
            true
        );
    }
}

void EventManager::onReceive(vir::Event::MouseButtonPressEvent& e)
{
    (void)e;
}

void EventManager::onReceive(vir::Event::MouseMotionEvent& e)
{
    (void)e;
}
 
void EventManager::onReceive(vir::Event::MouseButtonReleaseEvent& e)
{
    (void)e;
}

void EventManager::onReceive(vir::Event::KeyPressEvent& e)
{
    (void)e;
}

void EventManager::onReceive(vir::Event::KeyReleaseEvent& e)
{
    (void)e;
}

}