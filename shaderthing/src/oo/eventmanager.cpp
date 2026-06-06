#include "shaderthing/include/oo/eventmanager.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/corelogic.h"
#include "vir/include/vir.h"

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
            layer, 
            resolution, 
            appData_.rendering.isTiledRenderingEnabled, 
            true
        );
    }
}

void EventManager::onReceive(vir::Event::MouseButtonPressEvent& event)
{
    auto& su = *(appData_.sharedUniforms);
    if (!su.isCameraMouseInputEnabled)
        event.handled = true; // Prevent propagation to vir::InputCamera
    if (!su.isMouseInputEnabled)
        return;
    glm::vec4 mouse = 
    {
        event.x,
        su.iResolution.y-event.y,
        su.iMouse.x,
        -su.iMouse.y
    };
    if (su.isMouseInputClampedToWindow)
    {
        mouse.x = std::max(std::min(mouse.x, su.iResolution.x), 0.f);
        mouse.y = std::max(std::min(mouse.y, su.iResolution.y), 0.f);
    }
    if (su.iMouse == mouse)
        return;
    su.iUserAction = true;
    su.iMouse = mouse;
    su.fragment.uniformBuffer->markUniformForSubmission(su.iUserActionUniform.get());
    su.fragment.uniformBuffer->markUniformForSubmission(su.iMouseUniform.get());
    //su.updateDataRangeII = true;
}

void EventManager::onReceive(vir::Event::MouseMotionEvent& event)
{
    auto& su = *(appData_.sharedUniforms);
    bool LMBClicked = 
        vir::InputState::instance()->mouseButtonState(VIR_MOUSE_BUTTON_1)
        .isClicked();
    if 
    (
        !su.isCameraMouseInputEnabled || 
        (su.cameraMouseInputRequiresLMBHold && !LMBClicked)
    )
        event.handled = true; // Prevent propagation to vir::InputCamera
    if 
    (
        !su.isMouseInputEnabled || 
        (su.mouseInputRequiresLMBHold && !LMBClicked)
    )
        return;
    glm::vec4 mouse = 
    {
        event.x,
        su.iResolution.y-event.y,
        su.iMouse.z,
        su.iMouse.w
    };
    if (su.isMouseInputClampedToWindow)
    {
        mouse.x = std::max(std::min(mouse.x, su.iResolution.x), 0.f);
        mouse.y = std::max(std::min(mouse.y, su.iResolution.y), 0.f);
    }
    if (su.iMouse == mouse)
        return;
    su.iUserAction = true;
    su.iMouse = mouse;
    su.fragment.uniformBuffer->markUniformForSubmission(su.iUserActionUniform.get());
    su.fragment.uniformBuffer->markUniformForSubmission(su.iMouseUniform.get());
    //su.updateDataRangeII = true;
}
 
void EventManager::onReceive(vir::Event::MouseButtonReleaseEvent& event)
{
    auto& su = *(appData_.sharedUniforms);
    if (!su.isCameraMouseInputEnabled)
        event.handled = true; // Prevent propagation to vir::InputCamera
    if (!su.isMouseInputEnabled)
        return;
    glm::vec4 mouse = 
    {
        su.iMouse.x,
        su.iMouse.y,
        su.iMouse.z*-1,
        su.iMouse.w
    };
    if (su.iMouse == mouse)
        return;
    su.iUserAction = true;
    su.iMouse.z = mouse.z;
    su.fragment.uniformBuffer->markUniformForSubmission(su.iUserActionUniform.get());
    su.fragment.uniformBuffer->markUniformForSubmission(su.iMouseUniform.get());
    //su.updateDataRangeII = true;
}

void EventManager::onReceive(vir::Event::KeyPressEvent& event)
{
    if 
    (
        event.keyCode == VIR_KEY_ESCAPE &&
        vir::Window::instance()->cursorStatus() == 
            vir::Window::CursorStatus::Captured
    ) // Un-capture mouse on ESC
    {
        setMouseCaptured(appData_, false);
    }
    auto stKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    if (stKeyCode > 256)
        return;
    auto& su = *(appData_.sharedUniforms);
    auto& data(su.iKeyboard[stKeyCode]);
    static auto* inputState = vir::InputState::instance();
    auto& status = inputState->keyState(event.keyCode);
    data.x = (int)status.isPressed();
    data.y = (int)status.isHeld();
    data.z = (int)status.isToggled();
    su.fragment.uniformBuffer->markArrayUniformRangeForSubmission
    (
        su.iKeyboardUniform.get(), 
        stKeyCode
    );
}

void EventManager::onReceive(vir::Event::KeyReleaseEvent& event)
{
    auto stKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    if (stKeyCode > 256)
        return;
    auto& su = *(appData_.sharedUniforms);
    auto& data(su.iKeyboard[stKeyCode]);
    static auto* inputState = vir::InputState::instance();
    data.x = 0;
    data.y = 0;
    data.z = (int)inputState->keyState(event.keyCode).isToggled();
    su.fragment.uniformBuffer->markArrayUniformRangeForSubmission
    (
        su.iKeyboardUniform.get(), 
        stKeyCode
    );
}

}