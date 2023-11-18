#include "vpch.h"

namespace vir
{

void InputState::reset()
{
    for (int i=0; i<VIR_N_KEYS; i++)
    {
        keyState_[i] = State::None;
        keyToggle_[i] = false;
    }
    for (int i=0; i<VIR_N_MOUSE_BUTTONS; i++)
        mouseButtonState_[i] = State::None;
}

void InputState::onReceive(Event::KeyPressEvent& event)
{
    State& key(keyState_[event.keyCode()]);
    bool& keyToggle(keyToggle_[event.keyCode()]);
    if (key == State::None) 
        keyToggle = !keyToggle;
    key = (key == State::None) ? State::Pressed : State::Held;
}

void InputState::onReceive(Event::KeyReleaseEvent& event)
{
    keyState_[event.keyCode()] = State::None;
}

void InputState::onReceive(Event::MouseButtonPressEvent& event)
{
    State& mouseButton(mouseButtonState_[event.button()]);
    mouseButton = (mouseButton == State::None) ? State::Pressed : State::Held;
}

void InputState::onReceive(Event::MouseButtonReleaseEvent& event)
{
    mouseButtonState_[event.button()] = State::None;
}

void InputState::onReceive(Event::MouseMotionEvent& event)
{
    mousePosition_.x = event.x();
    mousePosition_.y = event.y();
}

}