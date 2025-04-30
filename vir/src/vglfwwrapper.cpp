#include "vpch.h"
#include "vglfwwrapper.h"

namespace vir
{

bool GLFWWrapper::glfwInitialized_ = false;
unsigned int GLFWWrapper::glfwWrapperCount_ = 0;

GLFWWrapper::GLFWWrapper()
{
    glfwWrapperCount_++;
    if (!glfwInitialized_)
    {
        glfwInit();
        glfwInitialized_ = true;
    }
}

GLFWWrapper::~GLFWWrapper()
{
    glfwWrapperCount_--;
    if (glfwWrapperCount_ == 0)
    {
        glfwInitialized_ = false;
        glfwTerminate();
    }
}

}