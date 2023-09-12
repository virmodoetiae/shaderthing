#include "vpch.h"

namespace vir
{

void InputState::onReceive(Event::KeyPressEvent& event)
{
    keyPressed_[event.keyCode()] = true;
    modKeyPressed_[event.modCode()] = true;
}

void InputState::onReceive(Event::KeyReleaseEvent& event)
{
    keyPressed_[event.keyCode()] = false;
    modKeyPressed_[event.modCode()] = false;
}

void InputState::onReceive(Event::MouseButtonPressEvent& event)
{
    mouseButtonPressed_[event.button()] = true;
}

void InputState::onReceive(Event::MouseButtonReleaseEvent& event)
{
    mouseButtonPressed_[event.button()] = false;
}

void InputState::onReceive(Event::MouseMotionEvent& event)
{
    mousePosition_.x = event.x();
    mousePosition_.y = event.y();
}



}