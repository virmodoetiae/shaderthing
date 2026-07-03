#ifndef V_OPENGL_CONTEXT
#define V_OPENGL_CONTEXT

#include "vgraphics/vcore/vgraphicscontext.h"

class GLFWwindow;

namespace vir
{

class OpenGLContext : public GraphicsContext
{
protected:
    static bool gladInitialized_;
public:
    OpenGLContext(void* nativeWindow);
    OpenGLContext() = delete;
    std::vector<std::string> retrieveErrors() const override;
};

}


#endif