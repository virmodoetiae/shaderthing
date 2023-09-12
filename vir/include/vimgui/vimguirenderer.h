#ifndef V_IMGUI_RENDERER_H
#define V_IMGUI_RENDERER_H

#include "thirdparty/imgui/imgui.h"
#include "veventsystem/vreceiver.h"

namespace vir
{

class ImGuiRenderer : public Event::Receiver
{
protected:

    static bool initialized_;
    ImGuiRenderer();
    virtual void newFrameImpl() = 0;
    virtual void renderImpl() = 0;
    virtual bool setWindowIconImpl
    (
        const char* windowName, 
        const unsigned char* data, 
        uint32_t dataSize, 
        bool isDataRaw
    ) = 0;

public:

    static ImGuiRenderer* initialize();
    virtual ~ImGuiRenderer();

    DECLARE_RECEIVABLE_EVENTS
    (
        Event::Type::MouseButtonPress *
        Event::Type::MouseButtonRelease *
        Event::Type::KeyPress *
        Event::Type::KeyRelease *
        Event::Type::KeyChar
    );

    void onReceive(Event::MouseButtonPressEvent&) override;
    void onReceive(Event::MouseButtonReleaseEvent&) override;
    void onReceive(Event::KeyPressEvent&) override;
    void onReceive(Event::KeyReleaseEvent&) override;
    void onReceive(Event::KeyCharEvent&) override;

    // Static functions which call the overriden implementation of the 
    // GlobalPtr-wrapped object of the corresponding xxxImpl functions
    static void newFrame();
    static void render();
    static bool setWindowIcon
    (
        const char* windowName, 
        const unsigned char* data, 
        uint32_t dataSize, 
        bool isDataRaw
    );
};

}

#endif