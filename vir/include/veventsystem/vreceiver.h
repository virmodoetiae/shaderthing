#ifndef VRECEIVER_H
#define VRECEIVER_H

#include "veventsystem/vevent.h"

namespace vir
{

namespace Event
{

class Broadcaster; // Fwd declaration for Receiver

class Receiver : public ReceiverCore
{
private:
    
    // Id assigned by broadcaster when receiver tunes in for first time
    unsigned long long int receiverId_;

    // Priority freely editable, higher priority means the receiver will
    // receive events (and possibly handle/resolve them) before receivers
    // with lower priority
    int receiverPriority_ = 0;

public:

    // Constructors
    // Default
    Receiver() : ReceiverCore(), receiverId_(0), receiverPriority_(0){};
    virtual ~Receiver();

    // Construct given a priority
    Receiver(int priority):ReceiverCore(),receiverId_(0),
        receiverPriority_(priority){};
    
    // Tune in to or out of the only existing broadcaster instance, if it exists
    bool tuneIn();
    bool tuneOut();

    // Accessors/setters
    unsigned long long int& receiverId(){return receiverId_;}
    unsigned long long int receiverId() const{return receiverId_;}
    int& receiverPriority(){return receiverPriority_;}
    int receiverPriority() const {return receiverPriority_;}
    
    // Operators
    // For checking whether this receiver already exists when inserting it in 
    // lists/vectors of Receivers
    bool operator==(const Receiver& rhs) const;
    // For sorting lists/vectors of Receivers
    bool operator<(const Receiver& rhs) const;
    bool operator>(const Receiver& rhs) const;

};

}

}

#endif