#ifndef V_OPENGL_MISC_H
#define V_OPENGL_MISC_H

#include "thirdparty/glad/include/glad/glad.h"

namespace vir
{

// Wait for all OpenGL commands issued up to this point to execute
void OpenGLWaitSync();

//
GLuint findFreeSSBOBindingPoint();

// Set the canRunOnDeviceInUse_, errorMessage_ variables of OpenGL-based
// PostProcess-derived objects
#define CHECK_OPENGL_COMPUTE_SHADERS_AVAILABLE                               \
    auto context = Window::instance()->context();                            \
    if (context->versionMajor() < 4)                                         \
        canRunOnDeviceInUse_ = false;                                        \
    else if (context->versionMinor() < 3)                                    \
        canRunOnDeviceInUse_ = false;                                        \
    if (!canRunOnDeviceInUse_)                                               \
    {                                                                        \
        auto* context(Window::instance()->context());                        \
        std::string glVersion                                                \
        (                                                                    \
            std::to_string(context->versionMajor())+"."+                     \
            std::to_string(context->versionMinor())                          \
        );                                                                   \
        std::string deviceName(Renderer::instance()->deviceName());          \
        errorMessage_ =                                                      \
"This feature requires an OpenGL version >= 4.3, but your graphics card in\n"\
"use ("+deviceName+") only supports OpenGL up to version "+glVersion;        \
        return;                                                              \
    }

}

#endif