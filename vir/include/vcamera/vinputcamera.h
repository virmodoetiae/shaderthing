#ifndef V_INPUT_CAMERA_H
#define V_INPUT_CAMERA_H

#include "vcamera/vcamera.h"
#include "veventsystem/vevent.h"

namespace vir
{

class InputCamera : public Camera, public Event::Receiver
{
protected:
    
    float dTheta_ = 0.0f;
    float dPhi_ = 0.0f;

public:

    InputCamera();

    DECLARE_RECEIVABLE_EVENTS
    (
        Event::Type::KeyPress *
        Event::Type::MouseMotion *
        Event::Type::MouseScroll *
        Event::Type::WindowResize // To keep up with aspect ratio changes
    );
    void onReceive(Event::KeyPressEvent&) override;
    void onReceive(Event::MouseMotionEvent&) override;
    void onReceive(Event::MouseScrollEvent&) override;
    void onReceive(Event::WindowResizeEvent&) override;

    void update() override;
};

}

#endif