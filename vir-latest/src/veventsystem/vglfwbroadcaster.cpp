#include "vpch.h"
#include "veventsystem/vglfwbroadcaster.h"

namespace vir
{

namespace Event
{

GLFWBroadcaster::GLFWBroadcaster() :
    GLFWWrapper(),
    GLFWwindow_(nullptr)
{
    auto p(Window::instance());
    if (p != nullptr)
        GLFWwindow_ = static_cast<GLFWwindow*>(p->nativeWindow());
    if (GLFWwindow_ != nullptr)
        bindToWindow(GLFWwindow_);
}

void GLFWBroadcaster::bindToWindow(GLFWwindow* window)
{
    GLFWwindow_ = window;
    
    // To enable accessing 'this' from within callback lambdas
    glfwSetWindowUserPointer(GLFWwindow_, this);
    
    // Callbacks for Key-related events
    glfwSetKeyCallback(
        GLFWwindow_, 
        [](GLFWwindow* window, int k, int s, int a, int m)
        {
            (void)s;
            (void)m;
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            int vk = inputKeyCodeGlfwToVir(k);
            int mk = inputKeyCodeGlfwToVir(m);
            switch(a)
            {
                case GLFW_PRESS :
                    this2->broadcast(KeyPressEvent(vk, mk, 0));
                    break;
                case GLFW_REPEAT :
                    this2->broadcast(KeyPressEvent(vk, mk, 1));
                    break;
                case GLFW_RELEASE :
                    this2->broadcast(KeyReleaseEvent(vk, mk));
                    break;
                default :
                    break;
            }
        }
    );

    glfwSetCharCallback
    (
        GLFWwindow_,
        [](GLFWwindow* window, unsigned int k)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            int vk = inputKeyCodeGlfwToVir(k);
            this2->broadcast(KeyCharEvent(vk));
        }
    );

    glfwSetMouseButtonCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window, int b, int a, int m)
        {
            (void)m;
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int vb = inputMouseCodeGlfwToVir(b);
            switch(a)
            {
                case GLFW_PRESS :
                    this2->broadcast(MouseButtonPressEvent(x, y, vb));
                    break;
                case GLFW_RELEASE :
                    this2->broadcast(MouseButtonReleaseEvent(x, y, vb));
                    break;
            }
        }
    );

    glfwSetCursorPosCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window, double x, double y)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(MouseMotionEvent(x, y));
        }
    );

    glfwSetScrollCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window, double dx, double dy)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(MouseScrollEvent(dx, dy));
        }
    );

    glfwSetWindowCloseCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowCloseEvent());
        }
    );

    glfwSetWindowFocusCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window, int focused)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowFocusEvent(bool(focused)));
        }
    );

    glfwSetWindowMaximizeCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window, int m)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowMaximizationEvent(bool(m)));
        }
    );

    glfwSetWindowIconifyCallback
    (
        GLFWwindow_, 
        [](GLFWwindow* window, int iconified)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowIconificationEvent(bool(iconified)));
        }
    );

    glfwSetWindowPosCallback
    (
        GLFWwindow_,
        [](GLFWwindow* window, int x, int y)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowMotionEvent(x, y));
        }
    );

    glfwSetFramebufferSizeCallback
    (
        GLFWwindow_,
        [](GLFWwindow* window, int w, int h)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowResizeEvent(w, h));
        }
    );

    glfwSetWindowContentScaleCallback
    (
        GLFWwindow_,
        [](GLFWwindow* window, float x, float y)
        {
            auto this2 = static_cast<GLFWBroadcaster*>
            (
                glfwGetWindowUserPointer(window)
            );
            this2->broadcast(WindowContentRescaleEvent(x, y));
        }
    );
}

void GLFWBroadcaster::broadcastNativeQueue()
{
    glfwPollEvents();
}

}

}