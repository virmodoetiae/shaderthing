#ifndef V_GLFW_OPENGL_WINDOW_H
#define V_GLFW_OPENGL_WINDOW_H

#include "vglfwwrapper.h"
#include "vwindow/vwindow.h"
#include "thirdparty/glm/glm.hpp"

namespace vir
{

class GLFWOpenGLWindow : public Window, public GLFWWrapper
{
private :

    GLFWwindow* glfwWindow_;

public :
    
    GLFWOpenGLWindow
    (
        uint32_t w, 
        uint32_t h, 
        std::string name, 
        bool resizable=true
    );
    
    void* nativeWindow() override {return glfwWindow_;}

    glm::vec2 contentScale() override;

    glm::ivec2 primaryMonitorResolution() override;

    void setTitle(std::string) override;

    //
    void setIcon
    (
        unsigned char* data, 
        uint32_t dataSize, 
        bool isDataRaw
    ) override;

    void setVSync(bool) override;

    void setViewport(uint32_t, uint32_t) override;

    // Never call from within an onReceive function for a WindowResizeEvent,
    // else the program may end up in an infinite loop
    void setSize(uint32_t, uint32_t) override;

    void setMouseCaptured(bool flag) override;

    void data(unsigned char*) override;

    bool isOpen() override;

    void update(bool swapBuffers=true) override;

    // Separate events by *, then delcare the appropriately overridden 
    // onReceive()
    DECLARE_RECEIVABLE_EVENTS
    (
        Event::Type::WindowResize *
        Event::Type::WindowFocus *
        Event::Type::WindowIconification *
        Event::Type::WindowMaximization *
        Event::Type::WindowClose
    )
    void onReceive(Event::WindowResizeEvent& e) override;
    void onReceive(Event::WindowFocusEvent& e) override;
    void onReceive(Event::WindowIconificationEvent& e) override;
    void onReceive(Event::WindowMaximizationEvent& e) override;
    void onReceive(Event::WindowCloseEvent& e) override;
};

}

#endif