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

    // Receiver addition and removal operator
    bool addReceiver(Receiver&);
    bool removeReceiver(Receiver&);

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

#define ONE_BROADCAST                                                       \
        Receiver* r = receivers[i];                                         \
        if (!r->isEventReceptionPaused(event.getType()))                    \
        {                                                                   \
            r->onReceive(event);                                            \
            if (event.handled)                                              \
                break;                                                      \
        }

        if (!broadcastInReversedOrder_)
        {
            for (int i=0; i<n; i++)
            {
                ONE_BROADCAST
            }
        }
        else
        {
            for (int i=n-1; i>=0; i--)
            {
                ONE_BROADCAST
            }
        }
    }

    // Same, but enable passing r-value references to events
    template <typename TE>
    void broadcast(TE&& event){broadcast(event);}

    // Accessors
    bool broadcastInReversedOrder() const {return broadcastInReversedOrder_;}
    bool& broadcastInReversedOrder() {return broadcastInReversedOrder_;}

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