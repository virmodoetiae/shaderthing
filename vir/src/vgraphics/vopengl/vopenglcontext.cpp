#include "vpch.h"
#include "vgraphics/vopengl/vopenglcontext.h"

namespace vir
{

bool OpenGLContext::gladInitialized_ = false;

void OpenGLContext::initialize(void* nativeWindow)
{
    glfwWindow_ = static_cast<GLFWwindow*>(nativeWindow);
    glfwMakeContextCurrent(glfwWindow_);
    if (!gladInitialized_)
    {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            throw std::exception();
        }
        gladInitialized_ = true;
    }
    const unsigned char* glr(glGetString(GL_RENDERER));
    std::cout << "Rendered: ";
    std::cout << glr << std::endl;
}

void OpenGLContext::swapBuffers()
{
    // It seems that at any point there are two buffers for the screen
    // pixels: one is the one used for drawing only (not visible), the other
    // is the one actually rendered to the screen (not operated on). So, the
    // program writes on the hidden buffer while the visibile one is 
    // displayed on screen, and at the next frame, when the writing to the
    // hidden buffer has finished, they are swapped so that we get a screen
    // updated. This is called double buffer. The visible is know as the
    // front buffer, the hidden as back buffer
    glfwSwapBuffers(glfwWindow_);
}

void OpenGLContext::printErrors() const
{
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        switch (err)
        {
        case GL_INVALID_ENUM:
            std::cout << "GL_INVALID_ENUM" << std::endl;
            break;
        case GL_INVALID_VALUE:
            std::cout << "GL_INVALID_VALUE" << std::endl;
            break;
        case GL_INVALID_OPERATION:
            std::cout << "GL_INVALID_OPERATION" << std::endl;
            break;
        case GL_STACK_UNDERFLOW:
            std::cout << "GL_STACK_UNDERFLOW" << std::endl;
            break;
        case GL_OUT_OF_MEMORY:
            std::cout << "GL_OUT_OF_MEMORY" << std::endl;
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << std::endl;
            break;
        case GL_CONTEXT_LOST:
            std::cout << "GL_CONTEXT_LOST" << std::endl;
            break;
        }
    }
}

}