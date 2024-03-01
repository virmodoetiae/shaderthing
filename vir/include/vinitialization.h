#ifndef V_INITIALIZATION_H
#define V_INITIALIZATION_H

#include <string>

namespace vir
{

enum class PlatformType
{
    None,
    GLFWOpenGL
};

extern PlatformType platform;

// Initial settings for the initialization of the vir backend via
// vir::initialize(Settings&). Please note that the plaftorm type and the window
// resizable flag cannot be changed after initialization
struct Settings
{
    const PlatformType platform                = PlatformType::GLFWOpenGL;
    const bool         enableWindowResizing    = true;
    
          unsigned int width                   = 512;
          unsigned int height                  = 512;
          std::string  windowName              = "main.exe";
          bool         initializeImGuiRenderer = true;
          bool         enableBlending          = true;
          bool         enableDepthTesting      = true;
          bool         enableFaceCulling       = true;
};

// Initialize the vir back-end with the provided settings
void initialize(const Settings& settings);

}

#endif