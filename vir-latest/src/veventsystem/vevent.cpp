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
    receivableEvents_(0)
{}

Receiver::~Receiver()
{
    tuneOutFromEventBroadcaster();
}

bool Receiver::tuneIntoEventBroadcaster(int priorityValue)
{
    Broadcaster* broadcaster(Broadcaster::instance());
    if (broadcaster == nullptr)
        return false;
    initialize();
    bool added = broadcaster->addReceiver(*this);
    if (added)
        setEventReceiverPriority(priorityValue);
    return added;
}

bool Receiver::tuneOutFromEventBroadcaster()
{
    Broadcaster* broadcaster(Broadcaster::instance());
    if (broadcaster != nullptr)
        return broadcaster->removeReceiver(*this);
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
    return canReceiveEvent(t) && receptionCooldownByEvent_[t] != 0;
}

void Receiver::resumeEventReception(Type t)
{
    if (canReceiveEvent(t))
        receptionCooldownByEvent_[t] = 0;
}

void Receiver::pauseEventReception(Type t, unsigned int cooldown)
{
    if (canReceiveEvent(t))
        receptionCooldownByEvent_[t] = cooldown == 0 ? -1 : cooldown;
}

void Receiver::setEventReceiverPriority(unsigned int value)
{
    Broadcaster* broadcaster(Broadcaster::instance());
    for (auto& entry : priorityByEvent_)
    {
        entry.second = value;
        if (broadcaster != nullptr)
            broadcaster->requestReceiverSorting(entry.first);
    }
}

void Receiver::setEventReceiverPriority(Type t, unsigned int value)
{
    Broadcaster* broadcaster(Broadcaster::instance());
    priorityByEvent_[t] = value;
    if (broadcaster != nullptr)
        broadcaster->requestReceiverSorting(t);
}

}

}