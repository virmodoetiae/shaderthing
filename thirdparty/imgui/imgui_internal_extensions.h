#pragma once

#include "imgui_internal.h"

namespace ImGui
{

extern float DRAG_SLIDER_LOGARITHMIC_ZERO_E;

template<typename TYPE>
IMGUI_API float CalcDragSliderLogarithmicZero
(
    const ImGuiDataType data_type, 
    const TYPE v_min, 
    const TYPE v_max, 
    const char* format
)
{
    int decimal_precision = 
        (data_type==ImGuiDataType_Float || data_type==ImGuiDataType_Double) ? 
        ImParseFormatPrecision(format, 3) : 1;
    if (decimal_precision == -1) // i.e., if exponential notation is used
    {
        if (v_min*v_max > 0) // Then use v_min as the log zero value
            return std::max(std::abs((float)v_min), FLT_MIN);
        // Else if the v bounds cross 0 and a custom log zero was set via 
        // PushDragSliderLogZeroForScientificNotation(), use that one
        else if (DRAG_SLIDER_LOGARITHMIC_ZERO_E != 0.f) 
            return DRAG_SLIDER_LOGARITHMIC_ZERO_E;
        // Else if the v bounds cross 0 but no custom log zero was set, force
        // the longest range on either side of the 0 to span 4 decades as a 
        // default
        else 
            return ImPow(.1f, 4-log10f(std::max((float)(v_max-v_min), FLT_MIN)));
    }
    else
        return ImPow(.1f, (float)decimal_precision);
}

}