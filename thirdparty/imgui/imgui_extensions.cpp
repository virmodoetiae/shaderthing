#include "imgui_extensions.h"
#include "imgui_internal_extensions.h"

void ImGui::PushDragSliderLogZeroForScientificNotation(float logarithmic_zero)
{
    logarithmic_zero = logarithmic_zero > 0 ? logarithmic_zero : -logarithmic_zero;
    if (logarithmic_zero < FLT_MIN)
        logarithmic_zero = FLT_MIN;
    DRAG_SLIDER_LOGARITHMIC_ZERO_E = logarithmic_zero;
}

void ImGui::PopDragSliderLogZeroForScientificNotation()
{
    DRAG_SLIDER_LOGARITHMIC_ZERO_E = 0.f;
}