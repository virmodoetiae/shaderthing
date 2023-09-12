#ifndef V_GRAPHICS_CONTEXT
#define V_GRAPHICS_CONTEXT

#include <iostream>

namespace vir
{

class GraphicsContext
{
public:
    enum class Type
    {
        OpenGL
    };
public:
    GraphicsContext()
    {
        #if DEBUG
        std::cout << "Graphics context constructor" << std::endl; 
        #endif
    }
    virtual ~GraphicsContext()
    {
        #if DEBUG
        std::cout << "Graphics context destroyed" << std::endl;
        #endif
    }
    virtual Type type() const = 0;
    virtual void initialize(void* nativeWindow) = 0;
    virtual void swapBuffers() = 0;
    virtual void printErrors() const = 0;
};

}

#endif