#ifndef VGLFWEVENTBROADCASTER_H
#define VGLFWEVENTBROADCASTER_H

#include "vglfwwrapper.h"
#include "veventsystem/vbroadcaster.h"

namespace vir
{

namespace Event
{

class GLFWBroadcaster : public Broadcaster, public GLFWWrapper
{
private:
    GLFWwindow* GLFWwindow_;
public:
    GLFWBroadcaster();
    void bindToWindow(GLFWwindow*);
};

}

}

#endif