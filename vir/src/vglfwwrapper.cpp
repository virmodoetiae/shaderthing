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
    #if (DEBUG)
    std::cout << "Created GLWWrapper, current count " 
        << glfwWrapperCount_ << std::endl;
    #endif
}

GLFWWrapper::~GLFWWrapper()
{
    glfwWrapperCount_--;
    if (glfwWrapperCount_ == 0)
    {
        glfwInitialized_ = false;
        glfwTerminate();
        #if (DEBUG)
        std::cout << "GLFW terminated " << std::endl;
        #endif
    }
    #if (DEBUG)
    std::cout << "GLWWrapper destroyed, current count " 
        << glfwWrapperCount_ << std::endl;
    #endif
}

}