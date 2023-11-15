#include "vpch.h"

namespace vir
{

namespace Event
{

Receiver::~Receiver()
{
    tuneOut();
}

bool Receiver::tuneIn()
{
    Broadcaster* p(GlobalPtr<Broadcaster>::instance());
    if (p != nullptr)
        return p->addReceiver(*this);
    return false;
}

bool Receiver::tuneOut()
{
    Broadcaster* p(GlobalPtr<Broadcaster>::instance());
    if (p != nullptr)
        return p->delReceiver(*this);
    return false;
    
}

// Operators
    
// For checking whether this receiver already exists when inserting it in 
// lists/vectors of Receivers
bool Receiver::operator==(const Receiver& rhs) const
{
    return (receiverId_ == rhs.receiverId());
}

// For sorting lists/vectors of Receivers
bool Receiver::operator<(const Receiver& rhs) const
{
    return (receiverPriority_ < rhs.receiverPriority());
}
bool Receiver::operator>(const Receiver& rhs) const
{
    return (receiverPriority_ > rhs.receiverPriority());
}

}

}