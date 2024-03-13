#include "vpch.h"
#include "vinitialization.h"
#include "vimgui/vimguirenderer.h"
#include "vimgui/vglfwopenglimguirenderer.h"

namespace vir
{

bool ImGuiRenderer::initialized_ = false;

ImGuiRenderer::ImGuiRenderer()
{   
    // Actual ImGui initialization stuff and settings
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       
    
    // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           
    
    // Enable Multi-Viewport / Platform Windows
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark(); //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

ImGuiRenderer::~ImGuiRenderer()
{
    ImGui::DestroyContext();
}

void ImGuiRenderer::onReceive(Event::MouseButtonPressEvent& event)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantCaptureMouse)
        event.handled = true;
}

void ImGuiRenderer::onReceive(Event::MouseButtonReleaseEvent& event)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantCaptureMouse)
        event.handled = true;
}

void ImGuiRenderer::onReceive(Event::KeyPressEvent& event)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantCaptureKeyboard)
        event.handled = true;
}

void ImGuiRenderer::onReceive(Event::KeyReleaseEvent& event)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantCaptureKeyboard)
        event.handled = true;
}

void ImGuiRenderer::onReceive(Event::KeyCharEvent& event)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantTextInput)
        event.handled = true;
}

ImGuiRenderer* ImGuiRenderer::initialize()
{
    if (initialized_)
        return ImGuiRenderer::instance();
    ImGuiRenderer* instance = nullptr;
    switch (vir::platform)
    {
        case PlatformType::GLFWOpenGL :
        {
            instance = GlobalPtr<ImGuiRenderer>::instance
            (
                new GLFWOpenGLImGuiRenderer()
            );
            break;
        }
        default :
        {
            throw std::runtime_error
            (
                "Invalid platform for ImGuiRenderer initialization"
            );
            break;
        }
    }
    if (instance != nullptr)
    {
        initialized_ = true;
        instance->tuneIntoEventBroadcaster(VIR_IMGUI_PRIORITY);
        return instance;
    }
    return nullptr;
}

void ImGuiRenderer::newFrame()
{
    ImGuiRenderer::instance()->newFrameImpl();
}

void ImGuiRenderer::render()
{
    ImGuiRenderer::instance()->renderImpl();
}

bool ImGuiRenderer::setWindowIcon
(
    const char* windowName, 
    const unsigned char* data, 
    uint32_t dataSize, 
    bool isDataRaw
)
{
    return ImGuiRenderer::instance()->setWindowIconImpl
    (
        windowName, 
        data, 
        dataSize, 
        isDataRaw
    );
}

void ImGuiRenderer::destroyDeviceObjects()
{
    ImGuiRenderer::instance()->destroyDeviceObjectsImpl();
}

}