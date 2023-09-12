#include "vpch.h"

namespace vir
{

namespace Event
{

bool MouseMotionEvent::first_ = true;
int MouseMotionEvent::x0_;
int MouseMotionEvent::y0_;

void ReceiverCore::initializeReceiver()
{
    for (Type et : allEventTypes)
    {
        if (canReceive(et))
            receivableEvents_.push_back(et);
    }
    receiverInitialized_ = true;
}

bool ReceiverCore::canReceive(Type et)
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

std::vector<Type>& ReceiverCore::receivableEvents() 
{
    if (!receiverInitialized_)
        initializeReceiver();
    return receivableEvents_;
}

}

}