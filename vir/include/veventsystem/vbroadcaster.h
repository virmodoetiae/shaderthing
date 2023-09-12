#ifndef VBROADCASTER_H
#define VBROADCASTER_H

#include <map>
#include "veventsystem/vevent.h"
#include "veventsystem/vreceiver.h"
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
    unsigned long long int receiverIdCounter_;

    //
    ReceiverPtrVector uniqueReceivers_;

    // Map of lists of receivers tuned in to this broadcaster, mapped by event
    // type. The same receiver can appear in multiple lists, if it can receive
    // or react to such event types
    std::map<Type, ReceiverPtrVector> receivers_;

    // Map of flags regarding the sorted status of each list of receivers, 
    // for each event Type. Have they already sorted by priority or not?
    std::map<Type, bool> sorted_;

    // 
    bool broadcastInReversedOrder_;

    // Default constructor
    Broadcaster();
    
    // TODO: disable other constructors

public:

    // D should be a derived class of Broadcaster
    template<typename D>
    static Broadcaster* initialize()
    {
        return GlobalPtr<Broadcaster>::instance(new D());
    }

    virtual ~Broadcaster();

    // Receiver addition and removal operator
    bool addReceiver(Receiver&);
    bool delReceiver(Receiver&);

    // Template function to broadcast any even type to the approriate onReceive
    // method implemented by all the receivers that can receive such an event
    // type
    template <typename TE>
    void broadcast(TE& event)
    {   
        Type t = TE::getStaticType();
        sortReceivers(t);
        ReceiverPtrVector& receivers(receivers_[t]);
        int n = receivers.size();
        for
        (
            int i = (broadcastInReversedOrder_) ? n-1 : 0;
            ((broadcastInReversedOrder_) ? (i >= 0) : (i <= n-1));
            ((broadcastInReversedOrder_) ? i-- : i++)
        )
        {
            Receiver* r = receivers[i];
            if (r->canCurrentlyReceive(event.getType()))
            {
                r->onReceive(event);
                if (event.handled())
                    break;
            }
        }
    }

    // Same, but enable passing r-value references to events
    template <typename TE>
    void broadcast(TE&& event){broadcast(event);}

    // Accessors
    bool broadcastInReversedOrder() const { return broadcastInReversedOrder_; }
    bool& broadcastInReversedOrder() { return broadcastInReversedOrder_; }

protected:

    // Find if a receiver already exists in a certain receiver list
    ReceiverPtrVector::iterator findReceiverIn
    (
        const Receiver&, 
        ReceiverPtrVector&
    );

    // Sort all receivers in the receivers list (for that event type) by
    // priority
    void sortReceivers(Type);
};

}

}

#endif