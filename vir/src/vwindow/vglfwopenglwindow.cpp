#include "vpch.h"
#include "vwindow/vglfwopenglwindow.h"
#include "vtime/vglfwtime.h"
#include "vgraphics/vbuffers.h"
#include "vgraphics/vopengl/vopenglcontext.h"
#include "thirdparty/stb/stb_image.h"

namespace vir
{

GLFWOpenGLWindow::GLFWOpenGLWindow
(
    uint32_t w, 
    uint32_t h, 
    std::string n, 
    bool r
) :
    Window(w, h, n, r)
{
    time_ = Time::initialize<GLFWTime>();
    context_ = new OpenGLContext();
    // OpenGL 4.3 minimum version supporting compute shaders
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    if (r)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    else
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindow_ = glfwCreateWindow(w, h, n.c_str(), NULL, NULL);
    if (glfwWindow_ == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        throw std::exception();
    }
    context_->initialize(glfwWindow_); 
    setVSync(true);
}

glm::vec2 GLFWOpenGLWindow::contentScale()
{
    float x, y;
    glfwGetWindowContentScale(glfwWindow_, &x, &y);
    return glm::vec2(x, y);
}

glm::ivec2 GLFWOpenGLWindow::primaryMonitorResolution()
{
    auto videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    return glm::ivec2(videoMode->width, videoMode->height);
}

void GLFWOpenGLWindow::setTitle(std::string title)
{
    title_ = title;
    glfwSetWindowTitle(glfwWindow_, title.c_str());
}

//
void GLFWOpenGLWindow::setIcon
(
    unsigned char* data, 
    uint32_t dataSize, 
    bool isDataRaw
)
{
    GLFWimage icon[1]; 
    int nc;
    if (isDataRaw)
        std::memcpy(icon[0].pixels, data, dataSize);
    else 
        icon[0].pixels = stbi_load_from_memory
        (
            data, 
            dataSize, 
            &icon[0].width, 
            &icon[0].height, 
            &nc, 
            4
        );
    glfwSetWindowIcon(glfwWindow_, 1, icon); 
    stbi_image_free(icon[0].pixels);
}

void GLFWOpenGLWindow::setVSync(bool state)
{
    if (state)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);
    VSync_ = state;
}

void GLFWOpenGLWindow::setViewport(uint32_t vwidth, uint32_t vheight)
{
    if (vwidth == viewportWidth_ && vheight == viewportHeight_)
        return;
    viewportWidth_ = vwidth;
    viewportHeight_ = vheight;
    glViewport(0, 0, viewportWidth_, viewportHeight_);
}

void GLFWOpenGLWindow::setSize(uint32_t width, uint32_t height)
{
    if (width == width_ && height == height_)
        return;
    width_ = width;
    height_ = height;
    setViewport(width, height);
    glfwSetWindowSize(glfwWindow_, width, height);
}

void GLFWOpenGLWindow::data(unsigned char* data)
{
    if (Framebuffer::activeOne() != nullptr)
        Framebuffer::activeOne()->unbind();
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

bool GLFWOpenGLWindow::isOpen()
{
    return !glfwWindowShouldClose(glfwWindow_);
}

void GLFWOpenGLWindow::update()
{
    time_->update();
    context_->swapBuffers();
    glfwPollEvents();
}

void GLFWOpenGLWindow::onReceive(Event::WindowResizeEvent& e)
{
    width_ = e.width();
    height_ = e.height();
    aspectRatio_ = float(width_)/float(height_);
    if (width_ != viewportWidth_ || height_ != viewportHeight_)
    {
        viewportWidth_ = width_;
        viewportHeight_ = height_;
        glViewport(0, 0, viewportWidth_, viewportHeight_);
    }
    //std::cout << "WindowResize " << width_ << " " << height_ << std::endl;
}

void GLFWOpenGLWindow::onReceive(Event::WindowFocusEvent& e)
{
    (void)e;
    //std::cout << "Window focus = " << e.gainedFocus() << std::endl;
}

void GLFWOpenGLWindow::onReceive(Event::WindowCloseEvent& e)
{
    (void)e;
    //std::cout << "Window closing " << std::endl;
}

}