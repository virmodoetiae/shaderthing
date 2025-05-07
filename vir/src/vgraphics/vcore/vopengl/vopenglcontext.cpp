#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglcontext.h"

namespace vir
{

bool OpenGLContext::gladInitialized_ = false;

OpenGLContext::OpenGLContext(void* nativeWindow) : 
GraphicsContext(GraphicsContext::Type::OpenGL)
{
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(nativeWindow));
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

std::vector<std::string> OpenGLContext::retrieveErrors() const
{
    GLenum error;
    std::vector<std::string> errors = {};
    while((error = glGetError()) != GL_NO_ERROR)
    {
        switch (error)
        {
        case GL_INVALID_ENUM:
            errors.emplace_back("GL_INVALID_ENUM");
            break;
        case GL_INVALID_VALUE:
            errors.emplace_back("GL_INVALID_VALUE");
            break;
        case GL_INVALID_OPERATION:
            errors.emplace_back("GL_INVALID_OPERATION");
            break;
        case GL_STACK_UNDERFLOW:
            errors.emplace_back("GL_STACK_UNDERFLOW");
            break;
        case GL_STACK_OVERFLOW:
            errors.emplace_back("GL_STACK_OVERFLOW");
            break;
        case GL_OUT_OF_MEMORY:
            errors.emplace_back("GL_OUT_OF_MEMORY");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errors.emplace_back("GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
        case GL_CONTEXT_LOST:
            errors.emplace_back("GL_CONTEXT_LOST");
            break;
        }
    }
    return errors;
}

}