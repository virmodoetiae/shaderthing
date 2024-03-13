#include "vpch.h"

namespace vir
{

namespace Event
{

bool MouseMotionEvent::first_ = true;
int MouseMotionEvent::x0_;
int MouseMotionEvent::y0_;

Receiver::Receiver() : 
    initialized_(false),
    priority_(0),
    receivableEvents_(0)
{}

Receiver::~Receiver()
{
    tuneOutFromEventBroadcaster();
}

bool Receiver::tuneIntoEventBroadcaster(int priorityValue)
{
    Broadcaster* p(Broadcaster::instance());
    if (p == nullptr)
        return false;
    initialize();
    bool added = p->addReceiver(*this);
    if (added)
        setEventReceiverPriority(priorityValue);
    return added;
}

bool Receiver::tuneOutFromEventBroadcaster()
{
    Broadcaster* p(Broadcaster::instance());
    if (p != nullptr)
        return p->removeReceiver(*this);
    return false;
}

void Receiver::initialize()
{
    if (initialized_)
        return;
    for (Type et : allEventTypes)
    {
        if (canReceiveEvent(et))
            receivableEvents_.push_back(et);
    }
    initialized_ = true;
}

bool Receiver::canReceiveEvent(Type et) const
{
    return 
    (
        std::fmod
        (
            receivableEventsCode(), 
            MIN_DOUBLE*et
        ) < MIN_DOUBLE
    );
}

bool Receiver::isEventReceptionPaused(Type t)
{
    return canReceiveEvent(t) && !currentlyReceivableEvents_[t];
}

void Receiver::resumeEventReception(Type t)
{
    if (canReceiveEvent(t))
        currentlyReceivableEvents_[t]=true;
}

void Receiver::pauseEventReception(Type t)
{
    if (canReceiveEvent(t))
        currentlyReceivableEvents_[t]=false;
}

}

}