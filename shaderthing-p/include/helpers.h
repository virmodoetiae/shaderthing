#pragma once

#include <vector>
#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

namespace Helpers
{

bool isCtrlKeyPressed(ImGuiKey key);

bool isCtrlShiftKeyPressed(ImGuiKey key);

glm::vec2 normalizedWindowResolution();

}

}