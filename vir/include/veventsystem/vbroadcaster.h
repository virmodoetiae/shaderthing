#ifndef VBROADCASTER_H
#define VBROADCASTER_H

#include <map>
#include "veventsystem/vevent.h"
#include "vglobalptr.h"

namespace vir
{

namespace Event
{

// Typedefs ------------------------------------------------------------------//

typedef std::vector<Receiver*> ReceiverPtrVector;

//

class Broadcaster
{
protected:

    // Counter used to assign the receiverId to newly added receivers
    unsigned int receiverIdCounter_              = 0;

    //
    ReceiverPtrVector uniqueReceivers_           = {};

    // Map of lists of receivers tuned in to this broadcaster, mapped by event
    // type. The same receiver can appear in multiple lists, if it can receive
    // or react to such event types
    std::map<Type, ReceiverPtrVector> receivers_ = {};

    // Map of flags regarding the sorted status of each list of receivers, 
    // for each event Type. Have they already sorted by priority or not?
    std::map<Type, bool> sorted_                 = {};

    // 
    bool broadcastInReversedOrder_               = false;

    Broadcaster() = default;

    // Delete copy/move constructors
    Broadcaster(const Broadcaster&) = delete;
    Broadcaster& operator=(const Broadcaster&) = delete;
    Broadcaster(Broadcaster&&) = delete;
    Broadcaster& operator=(Broadcaster&&) = delete;

public:

    // T should be a derived class of Broadcaster
    template<typename T>
    static Broadcaster* initialize()
    {
        return GlobalPtr<Broadcaster>::instance(new T());
    }

    virtual ~Broadcaster(){}

    // Add a receiver to this broadcaster, so that it will receive all events
    // of the types it supports receiving
    bool addReceiver(Receiver& receiver);

    // Remove a receiver from this broadcaster
    bool removeReceiver(Receiver& receiver);

    // Request that the broadcaster re-sort all receivers for an event type
    // t (only used in Receiver::setEventPriority) based on their priority
    void requestReceiverSorting(Type t);

    // Template function to broadcast any even type to the approriate onReceive
    // method implemented by all the receivers that can receive such an event
    // type
    template <typename TE>
    void broadcast(TE& event)
    {   
        Type t = TE::getStaticType();
        sortReceivers(t);
        ReceiverPtrVector& receivers(receivers_[t]);
        if (!broadcastInReversedOrder_)
        {
            for (int i=0; i<receivers.size(); i++)
            {
                Receiver* r = receivers[i];
                int& cooldown = r->receptionCooldownByEvent_[t];
                if (cooldown == 0)
                {
                    r->onReceive(event);
                    if (event.handled)
                        break;
                }
                else
                    cooldown -= cooldown > 0 ? 1 : 0;
            }
        }
        else
        {
            for (int i=receivers.size()-1; i>=0; i--)
            {
                Receiver* r = receivers[i];
                int& cooldown = r->receptionCooldownByEvent_[t];
                if (cooldown == 0)
                {
                    r->onReceive(event);
                    if (event.handled)
                        break;
                }
                else
                    cooldown -= cooldown > 0 ? 1 : 0;
            }
        }
    }

    // Template function to broadcast any even type to the approriate onReceive
    // method implemented by all the receivers that can receive such an event
    // type
    template <typename TE>
    void broadcast(TE&& event){broadcast(event);}

    //
    virtual void broadcastNativeQueue() = 0;

    // Accessors
    bool broadcastInReversedOrder() const {return broadcastInReversedOrder_;}
    bool& broadcastInReversedOrder() {return broadcastInReversedOrder_;}

    static Broadcaster* instance() {return GlobalPtr<Broadcaster>::instance();}

protected:

    // Find if a receiver already exists in a certain receiver list
    ReceiverPtrVector::iterator findReceiverIn
    (
        const Receiver& receiver, 
        ReceiverPtrVector& receiverPtrVector
    );

    // Sort all receivers in the receivers list (for that event type) by
    // priority
    void sortReceivers(Type t);
};

}

}

#endif