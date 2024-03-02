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

    struct Project
    {
        enum class Action
        {
            None,
            New,
            Load,
            Save,
            SaveAs
        };
        Action             action          = Action::None;
        std::string        filepath        = "";
        bool               forceSaveAs     = true;
    };
    Project                project_        = {};

    struct GUI
    {
        float*             fontScale       = nullptr;
    };
    GUI                    gui_            = {};

    SharedUniforms*        sharedUniforms_ = nullptr;
    std::vector<Layer*>    layers_         = {};
    std::vector<Resource*> resources_      = {};

    static FileDialog      fileDialog_;
    
    void saveProject(const std::string& filepath) const;
    void loadProject(const std::string& filepath);
    void newProject();

    void update();
    void initializeGUI();
    void renderGUI();
    void renderMenuBarGUI();
    
public:

    App();
    ~App();
};

}