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

class Event
{
protected:
    bool handled_;
public:
    Event():handled_(false){}
    bool handled() const {return handled_;}
    bool& handled(){return handled_;}
    virtual Type getType() = 0;
};

#define EVENT_IMPLEMENTS(type)                                              \
    Type getType() override {return Type::type;}                            \
    static constexpr Type getStaticType(){return Type::type;}

class KeyPressEvent : public Event
{
private:
    int keyCode_;
    int modCode_;
    int repeatCount_;
public:
    KeyPressEvent(int keyCode, int modCode, int repeatCount):
        keyCode_(keyCode),modCode_(modCode),repeatCount_(repeatCount){}
    EVENT_IMPLEMENTS(KeyPress)
    int keyCode(){return keyCode_;}
    int modCode(){return modCode_;}
    int repeatCount(){return repeatCount_;}
};

class KeyReleaseEvent : public Event
{
private:
    int keyCode_;
    int modCode_;
public:
    KeyReleaseEvent(int keyCode, int modCode):
        keyCode_(keyCode),modCode_(modCode){}
    EVENT_IMPLEMENTS(KeyRelease)
    int keyCode(){return keyCode_;}
    int modCode(){return modCode_;}
};

class KeyCharEvent : public Event
{
private:
    int keyCode_;
public:
    KeyCharEvent(int keyCode):keyCode_(keyCode){}
    EVENT_IMPLEMENTS(KeyChar)
    int keyCode(){return keyCode_;}
};

class MouseButtonPressEvent : public Event
{
private:
    int x_;
    int y_;
    int button_;
public:
    MouseButtonPressEvent(int x, int y, int button):
        x_(x),y_(y),button_(button){}
    EVENT_IMPLEMENTS(MouseButtonPress)
    int x(){return x_;}
    int y(){return y_;}
    int button(){return button_;}
};

class MouseButtonReleaseEvent : public Event
{
private:
    int x_;
    int y_;
    int button_;
public:
    MouseButtonReleaseEvent(int x, int y, int button):
        x_(x),y_(y),button_(button){}
    EVENT_IMPLEMENTS(MouseButtonRelease)
    int x(){return x_;}
    int y(){return y_;}
    int button(){return button_;}
};

class MouseMotionEvent : public Event
{
private:
    static bool first_;
    static int x0_;
    static int y0_;
    int x_;
    int y_;
    int dx_ = 0.0;
    int dy_ = 0.0;
public:
    MouseMotionEvent(int x, int y):x_(x),y_(y)
    {
        if (!first_)
        {
            dx_ = x_-x0_;
            dy_ = y_-y0_;
        }
        else
            first_ = false;
    }
    ~MouseMotionEvent(){x0_ = x_; y0_ = y_;}
    EVENT_IMPLEMENTS(MouseMotion)
    int x(){return x_;}
    int y(){return y_;}
    int dx(){return dx_;}
    int dy(){return dy_;}
};

class MouseScrollEvent : public Event
{
private:
    int dx_;
    int dy_;
public:
    MouseScrollEvent(int dx, int dy):dx_(dx),dy_(dy){}
    EVENT_IMPLEMENTS(MouseScroll)
    int dx(){return dx_;}
    int dy(){return dy_;}
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent(){}
    EVENT_IMPLEMENTS(WindowClose)
};

class WindowFocusEvent : public Event
{
private:
    bool gainedFocus_;
public:
    WindowFocusEvent(bool gainedFocus):gainedFocus_(gainedFocus){}
    EVENT_IMPLEMENTS(WindowFocus)
    bool gainedFocus(){return gainedFocus_;}
};

class WindowMaximizationEvent : public Event
{
private:
    // Ture if maximized, false if restored from maximum
    bool maximized_;
public:
    WindowMaximizationEvent(bool maximized):maximized_(maximized){}
    EVENT_IMPLEMENTS(WindowMaximization)
    bool maximized(){return maximized_;}
};

class WindowIconificationEvent : public Event
{
private:
    // Ture if iconified, false if restored from icon
    bool toIcon_;
public:
    WindowIconificationEvent(bool toIcon):toIcon_(toIcon){}
    EVENT_IMPLEMENTS(WindowIconification)
    bool toIcon(){return toIcon_;}
};

class WindowMotionEvent : public Event
{
private:
    int x_;
    int y_;
public:
    WindowMotionEvent(int x, int y):x_(x),y_(y){}
    EVENT_IMPLEMENTS(WindowMotion)
    int x(){return x_;}
    int y(){return y_;}
};

class WindowResizeEvent : public Event
{
private:
    int width_;
    int height_;
public:
    WindowResizeEvent(int width, int height):
        width_(width),height_(height){}
    EVENT_IMPLEMENTS(WindowResize)
    int width(){return width_;}
    int height(){return height_;}
};

class WindowContentRescaleEvent : public Event
{
private:
    int xScale_;
    int yScale_;
public:
    WindowContentRescaleEvent(int xScale, int yScale):
        xScale_(xScale),yScale_(yScale){}
    EVENT_IMPLEMENTS(WindowContentRescale)
    int xScale(){return xScale_;}
    int yScale(){return yScale_;}
};

class ProgramTickEvent : public Event
{
public :
    ProgramTickEvent(){}
    EVENT_IMPLEMENTS(ProgramTick)
};

class ProgramUpdateEvent : public Event
{
public :
    ProgramUpdateEvent(){}
    EVENT_IMPLEMENTS(ProgramUpdate)
};

class ProgramRenderEvent : public Event
{
public :
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