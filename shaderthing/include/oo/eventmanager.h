#pragma once

#include "vir/include/vir.h"

namespace ShaderThing
{

struct AppState;

class EventManager : public vir::Event::Receiver
{
private:

    AppState& appState_;

public:

    EventManager(AppState& appState);

    DECLARE_RECEIVABLE_EVENTS
    (
        vir::Event::Type::WindowResize *
        vir::Event::Type::MouseButtonPress *
        vir::Event::Type::MouseMotion *
        vir::Event::Type::MouseButtonRelease *
        vir::Event::Type::KeyPress *
        vir::Event::Type::KeyRelease
    )
    void onReceive(vir::Event::WindowResizeEvent& e) override;
    void onReceive(vir::Event::MouseButtonPressEvent& e) override;
    void onReceive(vir::Event::MouseMotionEvent& e) override;
    void onReceive(vir::Event::MouseButtonReleaseEvent& e) override;
    void onReceive(vir::Event::KeyPressEvent& e) override;
    void onReceive(vir::Event::KeyReleaseEvent& e) override;

};

}