#ifndef V_TIME_H
#define V_TIME_H

#include "vglobalptr.h"

namespace vir
{

class Time
{
public :
    enum class InnerTimestepType
    {
        Constant, // Never changes unless reset by user
        Adaptive // Tries to be as close to outerTimestep_ without exceeding a
                 // maximum value
    };
    enum class CouplingType
    {
        Decoupled, // innerTime_ and outerTime_ advance once per update_
        Coupled // innerTime_ tries to keep up with outerTime_ (via innerLoop)
    };
protected :
    float outerTime_ = 0.0f;
    float innerTime_ = 0.0f;
    float outerTimestep_ = 1.0f/60.0f;
    float innerTimestep_ = 1.0f/60.0f;
    float smoothOuterTimestep_ = 1.0f/60.0f;
    InnerTimestepType innerTimestepType_ = InnerTimestepType::Constant;
    CouplingType couplingType_ = CouplingType::Decoupled;
    float accumulator_ = 0.0f;
    float maxInnerTimestep_ = 0.0f;
    bool innerLoop_ = true;

public : 

    template<class DerivedTime>
    static Time* initialize()
    {
        return GlobalPtr<Time>::instance(new DerivedTime());
    }

    Time(){}

    virtual ~Time(){}

    void setConstantInnerTimeStep(float);
    void setAdaptiveInnerTimeStep(float);
    void setInnerOuterCoupling(CouplingType);

    virtual float now() = 0;

    void reset();
    bool innerLoop();
    void update();

    float outerTime(){return outerTime_;}
    float innerTime(){return innerTime_;}
    float outerTimestep(){return outerTimestep_;}
    float innerTimestep(){return innerTimestep_;}
    float smoothOuterTimestep(){return smoothOuterTimestep_;}
};

}

#endif