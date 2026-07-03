#include "vpch.h"
#include "vtime/vglfwtime.h"

namespace vir
{

float GLFWTime::now()
{
    return glfwGetTime();
}

}