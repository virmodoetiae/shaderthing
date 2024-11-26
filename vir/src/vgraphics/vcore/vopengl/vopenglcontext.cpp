#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglcontext.h"

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
    name_ = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    name_ = "OpenGL "+name_;
    GLint majorVersion, minorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    versionMajor_ = int(majorVersion);
    versionMinor_ = int(minorVersion);
    GLint nExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
    supportedExtensions_.resize(nExtensions);
    for (GLint i = 0; i < nExtensions; ++i)
    {
        std::string extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        supportedExtensions_[i] = extension;
    }
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