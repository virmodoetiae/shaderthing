#ifndef V_OPENGL_CONTEXT
#define V_OPENGL_CONTEXT

#include "vgraphics/vgraphicscontext.h"

class GLFWwindow;

namespace vir
{

class OpenGLContext : public GraphicsContext
{
protected:
    static bool gladInitialized_;
    GLFWwindow* glfwWindow_;
public:
    OpenGLContext() = default;
    Type type() const override {return Type::OpenGL;} 
    GLFWwindow* glfwWindow(){return glfwWindow_;}
    void initialize(void* nativeWindow) override;
    void printErrors() const override;
};

}


#endif