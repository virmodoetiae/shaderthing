#ifndef V_GLFW_OPENGL_IMGUI_RENDERER_H
#define V_GLFW_OPENGL_IMGUI_RENDERER_H

#include "vimgui/vimguirenderer.h"
#include <string>

namespace vir
{

class GLFWOpenGLImGuiRenderer : public ImGuiRenderer
{
protected:
    void newFrameImpl() override;
    void renderImpl() override;
    bool setWindowIconImpl
    (
        const char* windowName, 
        const unsigned char* data, 
        uint32_t dataSize, 
        bool isDataRaw
    ) override;
public:
    GLFWOpenGLImGuiRenderer();
    ~GLFWOpenGLImGuiRenderer();

};

}

#endif