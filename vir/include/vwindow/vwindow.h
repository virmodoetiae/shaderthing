#ifndef V_WINDOW_H
#define V_WINDOW_H

#include "veventsystem/vevent.h"
#include "vgraphics/vcore/vgraphicscontext.h"
#include "vglobalptr.h"

namespace vir
{

class Time;
class GraphicsContext;

class Window : public Event::Receiver
{
public :

    enum class PositionOf
    {
        TopLeftCorner = 0,
        TopRightCorner = 1,
        BottomLeftCorner = 2,
        BottomRightCorner = 3,
        Center = 4
    };

    enum class CursorStatus
    {
        Normal = 0,
        Hidden = 1,
        Captured = 2
    };

protected :

    // Time (not owned by Window, managed by GlobalPtr)
    Time* time_;

    // Graphics context (owned by Window)
    GraphicsContext* context_;

    // Window properties
    std::string title_;
    uint32_t width_;
    uint32_t height_;
    uint32_t viewportWidth_;
    uint32_t viewportHeight_;
    float aspectRatio_;
    bool iconified_;
    bool resizable_;
    bool VSync_;

public:

    Window(uint32_t, uint32_t, std::string, bool r=true);

    // Create window classes via this, where WT is the actual Window type to be 
    // instanced and CT the actual GraphicsContext type
    template<class WT>
    static Window* initialize
    (
        uint32_t w, 
        uint32_t h, 
        std::string t, 
        bool r=true
    )
    {
        return GlobalPtr<Window>::instance(new WT(w, h, t, r));
    }

    virtual ~Window();

    // Runs the provided function with the provided arguments on every window
    // frame while the window is open
    template<typename Function, typename... Args>
    void run(Function&& func, Args&&... args, const bool& swapBuffers)
    {
        while(this->isOpen())
        {
            func(std::forward<Args>(args)...);
            this->update(swapBuffers);
        }
    }

    // Runs the provided function on every window frame while the window is open
    template<typename Function>
    void run(Function&& func, const bool& swapBuffers=true)
    {
        while(this->isOpen())
        {
            func();
            this->update(swapBuffers);
        }
    }

    // Returns actual window-manager from chosen platform (e.g. GLFWwindow)
    virtual void* nativeWindow() = 0;

    //
    virtual glm::vec2 contentScale() = 0;

    //
    virtual glm::ivec2 position(PositionOf pof=PositionOf::TopLeftCorner)=0;

    //
    virtual glm::ivec2 primaryMonitorResolution() = 0;

    // 
    virtual void setTitle(std::string) = 0;

    //
    virtual void setIcon
    (
        unsigned char* data, 
        uint32_t dataSize, 
        bool isDataRaw
    ) = 0;

    // Enable or disable VSync
    virtual void setVSync(bool) = 0;

    // Set rendering viewport size
    virtual void setViewport(uint32_t, uint32_t) = 0;

    // Set window size and rendering viewport size, optinally broadcast the
    // resize as a WindowResizeEvent
    virtual void setSize(uint32_t, uint32_t) = 0;

    // 
    virtual void setCursorStatus(CursorStatus status) = 0;

    // 
    virtual CursorStatus cursorStatus() const = 0;

    // Retrieve window color data
    virtual void data(unsigned char*) = 0;

    // True as long as the window is open (or iconified)
    virtual bool isOpen() = 0;

    // Window update
    virtual void update(bool swapBuffers=true) = 0; 

    // Accessors
    Time* time() {return time_;}
    GraphicsContext* context() {return context_;}
    const std::string& title() const {return title_;}
    uint32_t width() const {return width_;}
    uint32_t height() const {return height_;}
    uint32_t viewportWidth() const {return viewportWidth_;}
    uint32_t viewportHeight() const {return viewportHeight_;}
    const float& aspectRatio() const {return aspectRatio_;}
    const bool& iconified() const {return iconified_;}
    const bool& resizable() const {return resizable_;}
    const bool& VSync() const {return VSync_;}

    //
    static Window* instance() {return GlobalPtr<Window>::instance();}
};

}

#endif