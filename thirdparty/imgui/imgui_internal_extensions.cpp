#include "imgui_internal_extensions.h"

// User-forced value for zero when using DragFloatN, SliderFloatN with 
// scientific notation enabled when the value bounds cross zero
float ImGui::DRAG_SLIDER_LOGARITHMIC_ZERO_E = 0.f;