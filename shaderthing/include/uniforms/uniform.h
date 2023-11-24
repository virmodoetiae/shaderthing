#ifndef ST_UNIFORM_H
#define ST_UNIFORM_H

#include "thirdparty/glm/glm.hpp"
#include "vir/include/vgraphics/vshader.h"

namespace ShaderThing
{

struct Uniform : public vir::Shader::Uniform
{
    typedef vir::Shader::Variable::Type Type;
    
    bool isShared = false;
    bool usesColorPicker = false;
    bool usesKeyboardInput = false;
    bool waitingForKeyboardInput = false;
    glm::vec2 limits = {0.f, 1.f};
    bool showLimits = true;
};

}

#endif