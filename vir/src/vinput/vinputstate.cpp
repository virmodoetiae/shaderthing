#include "vpch.h"

namespace vir
{

void InputState::onReceive(Event::KeyPressEvent& event)
{
    State& key(keyState_[event.keyCode()]);
    key = (key == State::None) ? State::Pressed : State::Held;
    State& modKey(modKeyState_[event.modCode()]);
    modKey = (modKey == State::None) ? State::Pressed : State::Held;
}

void InputState::onReceive(Event::KeyReleaseEvent& event)
{
    keyState_[event.keyCode()] = State::None;
    modKeyState_[event.modCode()] = State::None;
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