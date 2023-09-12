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
protected:
    
    bool keyPressed_[VIR_N_KEYS];
    bool modKeyPressed_[VIR_N_MOD_KEYS];
    bool mouseButtonPressed_[VIR_N_MOUSE_BUTTONS];
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
        return keyPressed_[keyCode];
    };
    bool isModKeyPressed(int modKeyCode) const
    {
        return modKeyPressed_[modKeyCode];
    };
    bool isMouseButtonPressed(int mouseButton)
    {
        return mouseButtonPressed_[mouseButton];
    }
    
    const bool* pressedKeys() const {return keyPressed_;}
    const bool* pressedModKeys() const {return modKeyPressed_;}
    const bool* pressedMouseButtons() const {return mouseButtonPressed_;}
    MousePosition& mousePosition(){return mousePosition_;}
};

}

#endif