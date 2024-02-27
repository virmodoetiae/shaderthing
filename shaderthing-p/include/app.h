#pragma once

#include <vector>
#include "vir/include/vir.h"
#include "shaderthing-p/include/macros.h"
#include "shaderthing-p/include/filedialog.h"

namespace ShaderThing
{

class SharedUniforms;
class Layer;
class Resource;
class FileDialog;
class ObjectIO;

class App
{
private:

    struct Flags
    {
        bool save = false;
        bool load = false;
    };
    Flags flags_ = {};

    struct GUI
    {
        float* fontScale;
    };
    GUI gui_ = {};

    SharedUniforms* sharedUniforms_;
    std::vector<Layer*> layers_ = {};
    std::vector<Resource*> resources_ = {};

    static FileDialog fileDialog_;
    
    void saveProject(const std::string& filepath) const;
    void openProject(const std::string& filepath);

    void update();
    void initializeGUI();
    void renderGUI();
    void renderMenuBarGUI();
    
public:

    App();
    ~App();
};

}