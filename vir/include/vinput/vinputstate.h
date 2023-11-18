#ifndef V_INPUT_STATE_H
#define V_INPUT_STATE_H

#include "vglobalptr.h"
#include "vinput/vinputcodes.h"
#include "veventsystem/vreceiver.h"

namespace vir
{

struct MousePosition
{
    float x;
    float y;
};

class InputState : public Event::Receiver
{
public :

    enum class State : unsigned char
    {
        None = 0,
        Pressed = 1,
        Held = 2
    };

protected:
    
    State keyState_[VIR_N_KEYS];
    State modKeyState_[VIR_N_MOD_KEYS];
    State mouseButtonState_[VIR_N_MOUSE_BUTTONS];
    MousePosition mousePosition_;
    InputState() = default;

public:

    static InputState* initialize()
    {
        return GlobalPtr<InputState>::instance(new InputState());
    }

    virtual ~InputState(){};

    DECLARE_RECEIVABLE_EVENTS
    (
        Event::Type::KeyPress *
        Event::Type::KeyRelease *
        Event::Type::MouseButtonPress *
        Event::Type::MouseButtonRelease *
        Event::Type::MouseMotion
    );
    void onReceive(Event::KeyPressEvent&) override;
    void onReceive(Event::KeyReleaseEvent&) override;
    void onReceive(Event::MouseButtonPressEvent&) override;
    void onReceive(Event::MouseButtonReleaseEvent&) override;
    void onReceive(Event::MouseMotionEvent&) override;

    bool isKeyPressed(int keyCode) const
    {
        return keyState_[keyCode] == State::Pressed;
    };
    bool isModKeyPressed(int modKeyCode) const
    {
        return modKeyState_[modKeyCode] == State::Pressed;
    };
    bool isMouseButtonPressed(int mouseButton)
    {
        return mouseButtonState_[mouseButton] == State::Pressed;
    }
    
    bool isKeyHeld(int keyCode) const
    {
        return keyState_[keyCode] == State::Held;
    };
    bool isModKeyHeld(int modKeyCode) const
    {
        return modKeyState_[modKeyCode] == State::Held;
    };
    bool isMouseButtonHeld(int mouseButton)
    {
        return mouseButtonState_[mouseButton] == State::Held;
    }
    
    bool isKeyPressedOrHeld(int keyCode) const
    {
        return keyState_[keyCode] != State::None;
    };
    bool isModKeyPressedOrHeld(int modKeyCode) const
    {
        return modKeyState_[modKeyCode] != State::None;
    };
    bool isMouseButtonPressedOrHeld(int mouseButton)
    {
        return mouseButtonState_[mouseButton] != State::None;
    }

    const State& keyState(int keyCode) const
    {
        return keyState_[keyCode];
    }
    const State& modKeyState(int modKeyCode) const
    {
        return modKeyState_[modKeyCode];
    }
    const State& mouseButtonState(int mouseButton) const
    {
        return mouseButtonState_[mouseButton];
    }
    
    //const State* const keyState() const {return keyState_;}
    //const State* const modKeyState() const {return modKeyState_;}
    //const State* const mouseButtonState() const {return mouseButtonState_;}
   
    MousePosition& mousePosition(){return mousePosition_;}
};

}

#endif