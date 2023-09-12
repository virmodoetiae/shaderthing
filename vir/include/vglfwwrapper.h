#ifndef VGLFWWRAPPER_H
#define VGLFWWRAPPER_H

// This define makes it so that the OpenGL headers are not loaded by GLFW, but
// by our chosen loader environment (GLAD, in our case, in pch.h)
#define GLFW_INCLUDE_NONE
// GLFW provides OS-independent bare necessities for creating an OpenGL context,
// windows, manage inputs, etc. It stands for Graphics Library FrameWork
#include "thirdparty/glfw3/include/GLFW/glfw3.h"

namespace vir
{

class GLFWWrapper
{
protected:
    static bool glfwInitialized_;
    static unsigned int glfwWrapperCount_;
public:
    GLFWWrapper();
    virtual ~GLFWWrapper();
    static bool glfwInitialized() {return glfwInitialized_;}
    static unsigned int glfwWrapperCount() {return glfwWrapperCount_;}
};

}

#endif