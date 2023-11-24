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

    class KeyState
    {
    friend InputState;
    private:
        bool pressed_ = false;
        bool held_ = false;
        bool toggled_ = false;
        void setPressed(bool flag){pressed_=flag; held_=false;}
        void setHeld(bool flag){held_=flag; pressed_=false;}
        void switchToggle(){toggled_=!toggled_;}
    public:
        void reset(){pressed_=false; held_=false; toggled_=false;}
        bool isPressed() const {return pressed_;}
        bool isHeld() const {return held_;}
        bool isPressedOrHeld() const {return pressed_ || held_ ;}
        bool isToggled() const {return toggled_;}
        const bool& pressedRef() const {return pressed_;}
        const bool& heldRef() const {return held_;}
        const bool& toggleRef() const {return toggled_;}
    };

    class MouseButtonState
    {
    friend InputState;
    private:
        bool clicked_ = false;
        bool toggled_ = false;
        void setClicked(bool flag){clicked_=flag;}
        void switchToggle(){toggled_ = !toggled_;}
    public:
        void reset(){clicked_=false; toggled_=false;}
        bool isClicked() const {return clicked_;}
        bool isToggled() const {return toggled_;}
        const bool& clickedRef() const {return clicked_;}
        const bool& toggleRef() const {return toggled_;}
    };


protected:
    
    KeyState keyState_[VIR_N_KEYS];
    MouseButtonState mouseButtonState_[VIR_N_MOUSE_BUTTONS];
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

    void reset();

    const KeyState& keyState(int keyCode) const 
    {
        return keyState_[keyCode];
    }

    const MouseButtonState& mouseButtonState(int mouseButtonCode) const 
    {
        return mouseButtonState_[mouseButtonCode];
    }
   
    const MousePosition& mousePosition(){return mousePosition_;}
};

}

#endif