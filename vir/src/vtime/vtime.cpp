#include "vpch.h"

namespace vir
{

void Time::setConstantInnerTimeStep(float dt)
{
    innerTimestepType_ = InnerTimestepType::Constant;
    innerTimestep_ = dt;
}

void Time::setAdaptiveInnerTimeStep(float maxDt)
{
    innerTimestepType_ = InnerTimestepType::Adaptive;
    maxInnerTimestep_ = maxDt;
}

void Time::setInnerOuterCoupling(CouplingType type)
{
    couplingType_ = type;
}

void Time::reset()
{
    outerTime_ = 0.0;
    innerTime_ = 0.0;
    accumulator_ = 0.0;
}

bool Time::innerLoop()
{
    bool shouldContinue = false;
    if (innerTimestepType_ == InnerTimestepType::Adaptive)
        innerTimestep_ = std::min(maxInnerTimestep_, outerTimestep_);
        
    /*
    If the inner and outer loops are decoupled, the innerTime will diverge from
    the outer (i.e. real world) time. The difference in adaptivity is that the
    adaptive timestep (the alternative to constant timestep) will try to set the
    inner timestep so that it is as close as possible to the outer time without
    exceeding a maximum. Also, for decoupled inner and outer time, the innerLoop
    should return true only once. Please also consider that, for a coupled case
    with a constant innerTimestep_ > (on average) the outerTimestep_, innerLoop
    will return true only after (on average) innerTimestep_ time has passed by
    the wall clock
    */
    if (couplingType_ == CouplingType::Decoupled)
    {
        if (innerLoop_)
        {
            shouldContinue = true;
            innerTime_ += innerTimestep_;
        }
        innerLoop_ = false;
    }
    else
    {
        if (accumulator_ > innerTimestep_)
        {
            accumulator_ -= innerTimestep_;
            shouldContinue = true;
            innerTime_ += innerTimestep_;
        }
        else
            innerLoop_ = false;
    }
    //if (shouldContinue)
    //    std::cout << " -> " << innerTime_ << " " << innerTimestep_ << " " 
    //        << accumulator_ << std::endl;
    return shouldContinue;
}

void Time::update()
{
    float currentOuterTime = now();
    outerTimestep_ = currentOuterTime-outerTime_;
    outerTime_ = currentOuterTime;
    smoothOuterTimestep_ = 0.01f*outerTimestep_ + 0.99f*smoothOuterTimestep_;
    
    if (couplingType_ == CouplingType::Coupled)
        accumulator_ += outerTimestep_;
    innerLoop_ = true;
}

}