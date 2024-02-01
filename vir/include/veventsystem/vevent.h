#ifndef VEVENT_H
#define VEVENT_H

#include <vector>
#include <unordered_map>
#include "vconstants.h"

namespace vir
{

namespace Event
{

// Event type enums ----------------------------------------------------------//

// Must map to prime numbers (any, order does not matter)
enum Type : unsigned long long int
{
    KeyPress = 2,               //
    KeyRelease = 3,             //
    KeyChar = 59,               //
    MouseButtonPress = 5,       //
    MouseButtonRelease = 7,     //
    MouseMotion = 11,           //
    MouseScroll = 13,           //
    WindowClose = 17,           //
    WindowFocus = 23,           //
    WindowMaximization = 29,    //
    WindowIconification = 31,   //
    WindowMotion = 37,          //
    WindowResize = 41,          //
    WindowContentRescale = 59,  //
    ProgramTick = 43,           //
    ProgramUpdate = 47,         //
    ProgramRender = 53          //
    // Missing
    // FramebufferResize (how is it different than WindowResize though?)
};
static const Type allEventTypes[] = 
{
    KeyPress,
    KeyRelease,
    KeyChar,
    MouseButtonPress,
    MouseButtonRelease,
    MouseMotion,
    MouseScroll,
    WindowClose,
    WindowFocus,
    WindowMaximization,
    WindowIconification,
    WindowMotion,
    WindowResize,
    WindowContentRescale,
    ProgramTick,
    ProgramUpdate,
    ProgramRender
};

// Event classes -------------------------------------------------------------//

struct Event
{
    bool handled = false;
    virtual Type getType() = 0;
};

#define EVENT_IMPLEMENTS(type)                                              \
    Type getType() override {return Type::type;}                            \
    static constexpr Type getStaticType(){return Type::type;}

struct KeyPressEvent : public Event
{
    int keyCode;
    int modCode;
    int repeatCount;
    KeyPressEvent(int keyCode, int modCode, int repeatCount):
        keyCode(keyCode),modCode(modCode),repeatCount(repeatCount){}
    EVENT_IMPLEMENTS(KeyPress)
};

struct KeyReleaseEvent : public Event
{
    int keyCode;
    int modCode;
    KeyReleaseEvent(int keyCode, int modCode):
        keyCode(keyCode),modCode(modCode){}
    EVENT_IMPLEMENTS(KeyRelease)
};

struct KeyCharEvent : public Event
{
    int keyCode;
    KeyCharEvent(int keyCode):keyCode(keyCode){}
    EVENT_IMPLEMENTS(KeyChar)
};

struct MouseButtonPressEvent : public Event
{
    int x;
    int y;
    int button;
    MouseButtonPressEvent(int x, int y, int button):
        x(x),y(y),button(button){}
    EVENT_IMPLEMENTS(MouseButtonPress)
};

struct MouseButtonReleaseEvent : public Event
{
    int x;
    int y;
    int button;
    MouseButtonReleaseEvent(int x, int y, int button):
        x(x),y(y),button(button){}
    EVENT_IMPLEMENTS(MouseButtonRelease)
};

struct MouseMotionEvent : public Event
{
private:
    static bool first_;
    static int x0_;
    static int y0_;
public:
    int x;
    int y;
    int dx = 0.0;
    int dy = 0.0;
    MouseMotionEvent(int x, int y):x(x),y(y)
    {
        if (!first_)
        {
            dx = x-x0_;
            dy = y-y0_;
        }
        else
            first_ = false;
    }
    ~MouseMotionEvent(){x0_ = x; y0_ = y;}
    EVENT_IMPLEMENTS(MouseMotion)
};

struct MouseScrollEvent : public Event
{
    int dx;
    int dy;
    MouseScrollEvent(int dx, int dy):dx(dx),dy(dy){}
    EVENT_IMPLEMENTS(MouseScroll)
};

struct WindowCloseEvent : public Event
{
    WindowCloseEvent(){}
    EVENT_IMPLEMENTS(WindowClose)
};

struct WindowFocusEvent : public Event
{
    const bool gainedFocus;
    WindowFocusEvent(bool gainedFocus):gainedFocus(gainedFocus){}
    EVENT_IMPLEMENTS(WindowFocus)
};

struct WindowMaximizationEvent : public Event
{
    // Ture if maximized, false if restored from maximum
    const bool maximized;
    WindowMaximizationEvent(bool maximized):maximized(maximized){}
    EVENT_IMPLEMENTS(WindowMaximization)
};

struct WindowIconificationEvent : public Event
{
    // False if restored from icon
    const bool iconified;
    WindowIconificationEvent(bool iconified):iconified(iconified){}
    EVENT_IMPLEMENTS(WindowIconification)
};

struct WindowMotionEvent : public Event
{
    int x;
    int y;
    WindowMotionEvent(int x, int y):x(x),y(y){}
    EVENT_IMPLEMENTS(WindowMotion)
};

struct WindowResizeEvent : public Event
{
    int width;
    int height;
    WindowResizeEvent(int width, int height):
        width(width),height(height){}
    EVENT_IMPLEMENTS(WindowResize)
};

struct WindowContentRescaleEvent : public Event
{
    int xScale;
    int yScale;
    WindowContentRescaleEvent(int xScale, int yScale):
        xScale(xScale),yScale(yScale){}
    EVENT_IMPLEMENTS(WindowContentRescale)
};

struct ProgramTickEvent : public Event
{
    ProgramTickEvent(){}
    EVENT_IMPLEMENTS(ProgramTick)
};

struct ProgramUpdateEvent : public Event
{
    ProgramUpdateEvent(){}
    EVENT_IMPLEMENTS(ProgramUpdate)
};

struct ProgramRenderEvent : public Event
{
    ProgramRenderEvent(){}
    EVENT_IMPLEMENTS(ProgramRender)
};

// End of Event classes ------------------------------------------------------//

// Receiver core -------------------------------------------------------------//

// To be used in classes derived from Receiver
#define DECLARE_RECEIVABLE_EVENTS(Type)                                        \
    double receivableEventsCode()                                              \
    {                                                                          \
        return vir::MIN_DOUBLE*Type;                                                \
    }

class ReceiverCore 
{
protected:

    bool receiverInitialized_;

    // List of immutable event types the derived Receiver object is intended
    // to be able to receive
    std::vector<Type> receivableEvents_;

    // List of mutable event types the derived Receiver object is currently
    // actively listening to (if and only if said event Type is present in
    // receivableEvents_)
    std::unordered_map<Type, bool> currentlyReceivableEvents_ =
    {
        {KeyPress, true},
        {KeyRelease, true},
        {KeyChar, true},
        {MouseButtonPress, true},
        {MouseButtonRelease, true},
        {MouseMotion, true},
        {MouseScroll, true},
        {WindowClose, true},
        {WindowFocus, true},
        {WindowMaximization, true},
        {WindowIconification, true},
        {WindowMotion, true},
        {WindowResize, true},
        {WindowContentRescale, true},
        {ProgramTick, true},
        {ProgramUpdate, true},
        {ProgramRender, true}
    };

public:

    // Constructor
    ReceiverCore() : receiverInitialized_(false), receivableEvents_(0){}

    // Pure virtuals
    virtual double receivableEventsCode() = 0;
    
    // Initialize the receivableEvents_ list based on the implementation of
    // receivableEventsCode
    void initializeReceiver();

    // True if this receiver can react to events of type t by design
    bool canReceive(Type t);

    // True if this receiver can is actively, currently listening to events of 
    // type et
    bool canCurrentlyReceive(Type t){return currentlyReceivableEvents_[t];}
    
    //
    void enableCurrentlyReceiving(Type t){currentlyReceivableEvents_[t]=true;}
    void disableCurrentlyReceiving(Type t){currentlyReceivableEvents_[t]=false;}

    // All possible onReceive signatures
    virtual void onReceive(KeyPressEvent& e){(void)e;}
    virtual void onReceive(KeyReleaseEvent& e){(void)e;}
    virtual void onReceive(KeyCharEvent& e){(void)e;}
    virtual void onReceive(MouseButtonPressEvent& e){(void)e;}
    virtual void onReceive(MouseButtonReleaseEvent& e){(void)e;}
    virtual void onReceive(MouseMotionEvent& e){(void)e;}
    virtual void onReceive(MouseScrollEvent& e){(void)e;}
    virtual void onReceive(WindowCloseEvent& e){(void)e;}
    virtual void onReceive(WindowFocusEvent& e){(void)e;}
    virtual void onReceive(WindowMaximizationEvent& e){(void)e;}
    virtual void onReceive(WindowIconificationEvent& e){(void)e;}
    virtual void onReceive(WindowMotionEvent& e){(void)e;}
    virtual void onReceive(WindowResizeEvent& e){(void)e;}
    virtual void onReceive(WindowContentRescaleEvent& e){(void)e;}
    virtual void onReceive(ProgramTickEvent& e){(void)e;}
    virtual void onReceive(ProgramUpdateEvent& e){(void)e;}
    virtual void onReceive(ProgramRenderEvent& e){(void)e;}

    // Accessors
    std::vector<Type>& receivableEvents();

};

} // End of namespace Event

} // End of namespace vir

#endif