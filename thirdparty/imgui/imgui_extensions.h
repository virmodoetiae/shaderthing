#pragma once

#include "imgui.h"

namespace ImGui
{

IMGUI_API void PushDragSliderLogZeroForScientificNotation(float logarithmic_zero);
IMGUI_API void PopDragSliderLogZeroForScientificNotation();

}