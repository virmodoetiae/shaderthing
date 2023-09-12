#ifndef V_GLFW_TIME_H
#define V_GLFW_TIME_H

#include "vtime/vtime.h"
#include "vglfwwrapper.h"

namespace vir
{

class GLFWTime : public Time, public GLFWWrapper
{
public: 
    float now() override;
};

}

#endif