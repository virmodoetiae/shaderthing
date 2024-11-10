#include "vpch.h"
#include "vwindow/vglfwopenglwindow.h"
#include "vtime/vglfwtime.h"
#include "vgraphics/vcore/vbuffers.h"
#include "vgraphics/vcore/vopengl/vopenglcontext.h"
#include "thirdparty/stb/stb_image.h"

namespace vir
{

GLFWOpenGLWindow::GLFWOpenGLWindow
(
    uint32_t width, 
    uint32_t height, 
    std::string name, 
    bool resizable
) :
    Window(width, height, name, resizable)
{
    time_ = Time::initialize<GLFWTime>();
    context_ = new OpenGLContext();

    // This is a very peculiar (read 'stupid') approach to finding the
    // highest supported OpenGL version on the system. Actually untested on
    // systems that do not support 4.6, so I am not sure this will work, but
    // in all honestly, we are talking about 5-6+ year old graphics cards (as of
    // late 2023)
    std::vector<std::pair<int, int>> glVersions = 
    {
        {4,6},
        {4,5},
        {4,4},
        {4,3}, // <- Minimum version supporting compute shaders
        {4,2},
        {4,1},
        {4,0},
        {3,3} // <- I don't care about supporting OpenGL versions below this one
    };
    for (auto glVersion : glVersions)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glVersion.first);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glVersion.second);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); 
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        if (resizable)
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        else
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindow_ = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
        if (glfwWindow_ != NULL)
            break;
    }
    if (glfwWindow_ == NULL)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
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

glm::ivec2 GLFWOpenGLWindow::position(PositionOf pof)
{
    int x, y;
    glfwGetWindowPos(glfwWindow_, &x, &y);
    switch (pof)
    {
    case PositionOf::TopLeftCorner :
        break;
    case PositionOf::TopRightCorner :
        x += width_;
        break;
    case PositionOf::BottomLeftCorner :
        y += height_;
        break;
    case PositionOf::BottomRightCorner :
        y += height_;
        x += width_;
        break;
    case PositionOf::Center :
        y += height_/2;
        x += width_/2;
        break;
    }
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
    // This is called automatically when the window is resized manually by
    // dragging its corners, but here, I need to call it myself
    glfwSetWindowSize(glfwWindow_, width, height);
    width_ = width;
    height_ = height;
    aspectRatio_ = float(width_)/float(height_);
    setViewport(width, height);
}

void GLFWOpenGLWindow::setCursorStatus(CursorStatus status)
{
    int glfwStatus;
    switch (status)
    {
        case CursorStatus::Normal :
            glfwStatus = GLFW_CURSOR_NORMAL;
            break;
        case CursorStatus::Hidden :
            glfwStatus = GLFW_CURSOR_HIDDEN;
            break;
        case CursorStatus::Captured :
            glfwStatus = GLFW_CURSOR_DISABLED;
            break;
    }
    glfwSetInputMode(glfwWindow_, GLFW_CURSOR, glfwStatus);
}

Window::CursorStatus GLFWOpenGLWindow::cursorStatus() const
{
    int mode = glfwGetInputMode(glfwWindow_, GLFW_CURSOR);
    if (mode == GLFW_CURSOR_NORMAL)
        return CursorStatus::Normal;
    if (mode == GLFW_CURSOR_HIDDEN)
        return CursorStatus::Hidden;
    if (mode == GLFW_CURSOR_DISABLED)
        return CursorStatus::Captured;
    return CursorStatus::Normal;
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

void GLFWOpenGLWindow::update(bool swapBuffers)
{
    time_->update();
    if (swapBuffers)
        glfwSwapBuffers(glfwWindow_);
    glfwPollEvents();
}

void GLFWOpenGLWindow::onReceive(Event::WindowResizeEvent& e)
{
    width_ = e.width;
    height_ = e.height;
    aspectRatio_ = float(width_)/float(height_);
    setViewport(width_, height_);
}

void GLFWOpenGLWindow::onReceive(Event::WindowFocusEvent& e)
{
    (void)e;
    iconified_ = false;
}

void GLFWOpenGLWindow::onReceive(Event::WindowIconificationEvent& e)
{
    iconified_ = e.iconified;
}

void GLFWOpenGLWindow::onReceive(Event::WindowMaximizationEvent& e)
{
    iconified_ = e.maximized;
}

void GLFWOpenGLWindow::onReceive(Event::WindowCloseEvent& e)
{
    (void)e;
}

}