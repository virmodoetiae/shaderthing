#pragma once

#include <vector>
#include "vir/include/vir.h"
#include "shaderthing-p/include/data/shareduniforms.h"

#define DELETE_COPY_MOVE(class)                                             \
    class(const class&)=delete;                                             \
    class& operator=(const class&)=delete;                                  \
    class(class&&)=delete;                                                  \
    class& operator=(class&&)=delete;

namespace ShaderThing
{

struct SharedUniformBlock;
struct Layer;

struct App
{
    struct Flags
    {

    };
    Flags flags = {};

    struct GUI
    {
        float* fontScale;
    };
    GUI gui = {};

    SharedUniforms sharedUniforms = {};
    std::vector<Layer*> layers = {};
    
    App();
    
    ~App(){}
};

}