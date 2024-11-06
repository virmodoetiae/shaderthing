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

// Receiver ------------------------------------------------------------------//

// To be used in classes derived from Receiver
#define DECLARE_RECEIVABLE_EVENTS(Type)                                        \
    double receivableEventsCode() const                                        \
    {                                                                          \
        return vir::MIN_DOUBLE*Type;                                           \
    }

class Broadcaster;

class Receiver
{

#define VIR_INPUT_PRIORITY 10
#define VIR_WINDOW_PRIORITY 20
#define VIR_IMGUI_PRIORITY 30
#define VIR_CAMERA_PRIORITY 50
#define VIR_DEFAULT_PRIORITY 100

friend Broadcaster;

private:

    //
    bool initialized_;

    // Id assigned by broadcaster when receiver tunes in for first time
    unsigned int id_;

    // Map to indicate what is the priority for receiving events of each
    // type as compared to other receivers. Smaller values mean higher
    // priority, i.e., events will be receiver earlier. 
    std::map<Type, int> priorityByEvent_ = 
    {
        {KeyPress,              VIR_DEFAULT_PRIORITY},
        {KeyRelease,            VIR_DEFAULT_PRIORITY},
        {KeyChar,               VIR_DEFAULT_PRIORITY},
        {MouseButtonPress,      VIR_DEFAULT_PRIORITY},
        {MouseButtonRelease,    VIR_DEFAULT_PRIORITY},
        {MouseMotion,           VIR_DEFAULT_PRIORITY},
        {MouseScroll,           VIR_DEFAULT_PRIORITY},
        {WindowClose,           VIR_DEFAULT_PRIORITY},
        {WindowFocus,           VIR_DEFAULT_PRIORITY},
        {WindowMaximization,    VIR_DEFAULT_PRIORITY},
        {WindowIconification,   VIR_DEFAULT_PRIORITY},
        {WindowMotion,          VIR_DEFAULT_PRIORITY},
        {WindowResize,          VIR_DEFAULT_PRIORITY},
        {WindowContentRescale,  VIR_DEFAULT_PRIORITY},
        {ProgramTick,           VIR_DEFAULT_PRIORITY},
        {ProgramUpdate,         VIR_DEFAULT_PRIORITY},
        {ProgramRender,         VIR_DEFAULT_PRIORITY}
    };

    // List of immutable event types the derived Receiver object is intended
    // to be able to receive by design, i.e., those declared via the
    // DECLARE_RECEIVABLE_EVENTS macro
    std::vector<Type> receivableEvents_;

    // List of mutable event types the derived Receiver object is currently
    // actively listening to (if and only if said event Type is present in
    // receivableEvents_)
    std::map<Type, bool> currentlyReceivableEvents_ =
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

    // Initialize the receivableEvents_ list based on the implementation of
    // receivableEventsCode
    void initialize();

public:

    // Constructor
    Receiver();

    virtual ~Receiver();

    // Register with the global event broadcaster to enable receiving events.
    // A priority can be specified as a default priority value for all event
    // types. Lower values mean higher priority (i.e., the receiver will receive
    // events before other receivers with higher priority values).  To set the 
    // priority for receiving specific events, use the setEventReceiverPriority(
    // Type t, unsigned int value) function
    bool tuneIntoEventBroadcaster(int priorityValue=VIR_DEFAULT_PRIORITY);
    
    // Unregister from the global event broadcaster to disable receiving events
    bool tuneOutFromEventBroadcaster();

    // Never override manually, always override via 
    // the DECLARE_RECEIVABLE_EVENTS macro
    virtual double receivableEventsCode() const = 0;

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

    // True if this receiver can react to events of type t by design. Please
    // note however that the reception of events of type t may be paused. To
    // poll current event reception status for event type t, use 
    // isEventReceptionPaused(t)
    bool canReceiveEvent(Type t) const;

    // True if this receiver can receive event type y by design but is not
    // currently capable to react to it. To resume listening to said event type,
    // use resumeEventReception(t)
    bool isEventReceptionPaused(Type t);
    
    // Resume listening to event type t
    void resumeEventReception(Type t);

    // Pause listening to event type t
    void pauseEventReception(Type t);

    // Retrieve the priority for receiving events of type t. Smaller values
    // indicate a higher priority. Values may be negative
    int eventReceiverPriority(Type t) const {return priorityByEvent_.at(t);}

    // Set the priority for receiving all events. Smaller values indicate a
    // higher priority. Values may be negative
    void setEventReceiverPriority(unsigned int value);

    // Set the priority for receiving events of type t. Smaller values
    // indicate a higher priority. Values may be negative
    void setEventReceiverPriority(Type t, unsigned int value);
};

} // End of namespace Event

} // End of namespace vir

#endif