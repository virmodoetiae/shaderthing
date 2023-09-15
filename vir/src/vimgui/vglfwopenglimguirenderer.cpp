#include "vpch.h"
#include "vimgui/vglfwopenglimguirenderer.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/backends/imgui_impl_glfw.h"
#include "thirdparty/imgui/backends/imgui_impl_opengl3.h"

namespace vir
{

GLFWOpenGLImGuiRenderer::GLFWOpenGLImGuiRenderer()
{
    ImGui_ImplGlfw_InitForOpenGL
    (
        static_cast<GLFWwindow*>
        (
            GlobalPtr<Window>::instance()->nativeWindow()
        ), 
        true
    );
    ImGui_ImplOpenGL3_Init("#version 410");
}

GLFWOpenGLImGuiRenderer::~GLFWOpenGLImGuiRenderer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

void GLFWOpenGLImGuiRenderer::newFrameImpl()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GLFWOpenGLImGuiRenderer::renderImpl()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    // for this specific demo app we could also call 
    // glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupCurrentContext);
    }
}

bool GLFWOpenGLImGuiRenderer::setWindowIconImpl
(
    const char* windowName, 
    const unsigned char* data, 
    uint32_t dataSize, 
    bool isDataRaw
)
{
    auto window = ImGui::FindWindowByName(windowName);
    if (window == NULL)
        return false;
    if (window->Viewport == NULL)
        return false;
    if (window->Viewport->PlatformHandle == NULL)
        return false;
    GLFWimage icon[1]; 
    int nc;
    if (isDataRaw)
        std::memcpy(icon[0].pixels, data, dataSize);
    else 
    {
        stbi_set_flip_vertically_on_load(false);
        icon[0].pixels = stbi_load_from_memory
        (
            data, 
            dataSize, 
            &icon[0].width, 
            &icon[0].height, 
            &nc, 
            4
        );
    }
    glfwSetWindowIcon((GLFWwindow*)window->Viewport->PlatformHandle, 1, icon);
    stbi_image_free(icon[0].pixels);
    return true;
}

void GLFWOpenGLImGuiRenderer::destroyDeviceObjectsImpl()
{
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
}

}