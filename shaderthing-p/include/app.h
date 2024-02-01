#pragma once

#include <vector>
#include "vir/include/vir.h"
#include "shaderthing-p/include/macros.h"

namespace ShaderThing
{

class SharedUniforms;
class Layer;

class App
{
private:

    struct Flags
    {

    };
    Flags flags_ = {};

    struct GUI
    {
        float* fontScale;
    };
    GUI gui_ = {};

    SharedUniforms* sharedUniforms_;
    std::vector<Layer*> layers_ = {};

    Layer* createLayer();
    void deleteLayer(Layer* layer);

    void update();
    void initializeGUI();
    void renderGUI();
    void renderMenuBarGUI();
    
public:

    App();
    ~App();
};

}