#include "vpch.h"

namespace vir
{

void InputState::reset()
{
    for (int i=0; i<VIR_N_KEYS; i++)
        keyState_[i].reset();
    for (int i=0; i<VIR_N_MOUSE_BUTTONS; i++)
        mouseButtonState_[i].reset();
}

void InputState::onReceive(Event::KeyPressEvent& event)
{
    KeyState& key(keyState_[event.keyCode()]);
    if (key.isPressedOrHeld())
        key.setHeld(true);
    else
    {
        key.switchToggle();
        key.setPressed(true);
    }
}

void InputState::onReceive(Event::KeyReleaseEvent& event)
{
    keyState_[event.keyCode()].setPressed(false);
}

void InputState::onReceive(Event::MouseButtonPressEvent& event)
{
    MouseButtonState& mouseButton(mouseButtonState_[event.button()]);
    mouseButton.switchToggle();
    mouseButton.setClicked(true);
}

void InputState::onReceive(Event::MouseButtonReleaseEvent& event)
{
    mouseButtonState_[event.button()].setClicked(false);
}

void InputState::onReceive(Event::MouseMotionEvent& event)
{
    mousePosition_.x = event.x();
    mousePosition_.y = event.y();
}

}