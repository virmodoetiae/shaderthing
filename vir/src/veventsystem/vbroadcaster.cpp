#include "vpch.h"

namespace vir
{

namespace Event
{

bool Broadcaster::addReceiver(Receiver& receiver)
{
    
    bool added = false;
    for (Type te : receiver.receivableEvents_)
    {
        if (!(receivers_.find(te)==receivers_.end()))
        {
            receivers_.insert
            (
                std::make_pair(te, ReceiverPtrVector())
            );
            sorted_.insert
            (
                std::make_pair(te, true) 
            );
        }
        ReceiverPtrVector& teReceivers(receivers_[te]);
        auto it = findReceiverIn(receiver, teReceivers);
        if (it == teReceivers.end())
        {
            if (!added) // First addition
            {
                receiver.id_ = ++receiverIdCounter_;
                uniqueReceivers_.push_back(&receiver);
            }
            teReceivers.push_back(&receiver);
            added = true;
            sorted_[te] = false;
        }
    }
    return added;
}

bool Broadcaster::removeReceiver(Receiver& receiver)
{
    bool removed = false;
    for (Type te : receiver.receivableEvents_)
    {
        ReceiverPtrVector& teReceivers(receivers_[te]);
        auto it = findReceiverIn(receiver, teReceivers);
        if (it != teReceivers.end())
        {
            teReceivers.erase(it);
            if (teReceivers.size() == 0)
                receivers_.erase(te);
            removed = true;
        }
    }
    if (removed)
    {
        auto it = findReceiverIn(receiver, uniqueReceivers_);
        if (it != uniqueReceivers_.end())
            uniqueReceivers_.erase(it);
    }
    return removed;
}

void Broadcaster::requestReceiverSorting(Type t)
{
    if (sorted_.find(t) != sorted_.end())
        sorted_[t] = false;
}

// Find if a receiver already exists in a certain receiver list
ReceiverPtrVector::iterator Broadcaster::findReceiverIn
(
    const Receiver& receiver, 
    ReceiverPtrVector& receivers
)
{
    const unsigned int& id(receiver.id_);
    ReceiverPtrVector::iterator it = 
        std::find_if
        (
            receivers.begin(), 
            receivers.end(), 
            [id](Receiver* e)
            {
                return (e->id_ == id);
            } 
        );
    return it;
}

void Broadcaster::sortReceivers(Type te)
{
    bool& sorted = sorted_[te];
    if (!sorted)
    {
        ReceiverPtrVector& receivers(receivers_[te]);
        std::sort
        (
            receivers.begin(), 
            receivers.end(), 
            [&te](Receiver* r0, Receiver* r1)
            {
                return 
                (
                    r0->eventReceiverPriority(te) < 
                    r1->eventReceiverPriority(te)
                );
            }
        );
        sorted = true;
    }
}

}

}